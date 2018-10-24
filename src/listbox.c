/* 
* +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
* |                   |  Enno Rehling <enno@eressea.de>
* | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
* | (c) 1998 - 2006   |
* |                   |  This program may not be used, modified or distributed
* +-------------------+  without prior permission by the authors of Eressea.
*
*/

#ifdef _MSC_VER
#include <platform.h>
#endif

#include <curses.h>

#include "listbox.h"
#include "gmtool_structs.h"

#include <util/strings.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void
insert_selection(list_selection ** p_sel, list_selection * prev,
const char *str, void *payload)
{
    list_selection *sel = (list_selection *)calloc(1, sizeof(list_selection));
    sel->str = str_strdup(str);
    sel->data = payload;
    if (*p_sel) {
        list_selection *s;
        sel->next = *p_sel;
        sel->prev = sel->next->prev;
        sel->next->prev = sel;
        if (sel->prev) {
            sel->prev->next = sel;
            sel->index = sel->prev->index + 1;
        }
        for (s = sel->next; s; s = s->next) {
            s->index = s->prev->index + 1;
        }
        *p_sel = sel;
    }
    else {
        *p_sel = sel;
        sel->prev = prev;
        if (prev)
            sel->index = prev->index + 1;
    }
}

list_selection **push_selection(list_selection ** p_sel, char *str,
    void *payload)
{
    list_selection *sel = (list_selection *)calloc(1, sizeof(list_selection));
    list_selection *prev = NULL;
    sel->str = str;
    sel->data = payload;
    while (*p_sel) {
        prev = *p_sel;
        p_sel = &prev->next;
    }
    *p_sel = sel;
    if (prev) {
        sel->prev = prev;
        sel->index = prev->index + 1;
    }
    return p_sel;
}

#define SX (getmaxx(stdscr))
#define SY (getmaxy(stdscr))

list_selection *do_selection(list_selection * sel, const char *title,
    void(*perform) (list_selection *, void *), void *data)
{
    WINDOW *wn;
    bool update = true;
    list_selection *s;
    list_selection *top = sel;
    list_selection *current = top;
    int i;
    int height = 0, width = (int)strlen(title) + 8;
    for (s = sel; s; s = s->next) {
        if ((int)strlen(s->str) > width) {
            width = (int)strlen(s->str);
        }
        ++height;
    }
    if (height == 0 || width == 0)
        return 0;
    if (width + 3 > SX)
        width = SX - 4;
    if (height + 2 > SY)
        height = SY - 2;

    wn =
        newwin(height + 2, width + 4, (SY - height - 2) / 2, (SX - width - 4) / 2);

    for (;;) {
        int input;
        if (update) {
            for (s = top; s != NULL && top->index + height != s->index; s = s->next) {
                i = s->index - top->index;
                wmove(wn, i + 1, 4);
                waddnstr(wn, s->str, -1);
                wclrtoeol(wn);
            }
            wclrtobot(wn);
            wxborder(wn);
            mvwprintw(wn, 0, 2, "[ %s ]", title);
            update = false;
        }
        i = current->index - top->index;
        wattron(wn, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
        wmove(wn, i + 1, 2);
        waddstr(wn, "->");
        wmove(wn, i + 1, 4);
        waddnstr(wn, current->str, width - 2);
        wattroff(wn, A_BOLD | COLOR_PAIR(COLOR_YELLOW));

        wrefresh(wn);

        input = getch();

        wmove(wn, i + 1, 2);
        waddstr(wn, "  ");
        wmove(wn, i + 1, 4);
        waddnstr(wn, current->str, width);

        switch (input) {
        case KEY_NPAGE:
            for (i = 0; i != height / 2; ++i) {
                if (current->next) {
                    current = current->next;
                    if (current->index - height >= top->index) {
                        top = current;
                        update = true;
                    }
                }
            }
            break;
        case KEY_PPAGE:
            for (i = 0; i != height / 2; ++i) {
                if (current->prev) {
                    if (current == top) {
                        top = sel;
                        while (top->index + height < current->index)
                            top = top->next;
                        update = true;
                    }
                    current = current->prev;
                }
            }
            break;
        case KEY_DOWN:
            if (current->next) {
                current = current->next;
                if (current->index - height >= top->index) {
                    top = current;
                    update = true;
                }
            }
            break;
        case KEY_UP:
            if (current->prev) {
                if (current == top) {
                    top = sel;
                    while (top->index + height < current->index)
                        top = top->next;
                    update = true;
                }
                current = current->prev;
            }
            break;
        case 27:
        case 'q':
            delwin(wn);
            return NULL;
        case 10:
        case 13:
            if (perform)
                perform(current, data);
            else {
                delwin(wn);
                return current;
            }
            break;
        default:
            s = current->next;
            if (s == NULL)
                s = top;
            while (s != current) {
                if (tolower(s->str[0]) == tolower(input)) {
                    current = s;
                    update = true;
                }
                else {
                    s = s->next;
                    if (s == NULL)
                        s = top;
                }
            }
            if (current->index - height >= top->index) {
                top = current;
                update = true;
            }
        }
    }
}
