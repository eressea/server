/* vi: set ts=2:
 *
 *	$Id: map_tools.c,v 1.6 2001/02/10 12:31:42 corwin Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <curses.h>
#include <eressea.h>
#include "mapper.h"

/* kernel includes */
#include <building.h>
#include <region.h>
#include <ship.h>
#include <unit.h>

/* util includes */
#include <base36.h>

/* libc includes */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
adddbllist(dbllist ** SP, const char *s)
{
	dbllist *S, *X;

	S = calloc(1, sizeof(dbllist) + strlen(s));
	strcpy(S->s, s);

	S->next = 0;

	if (*SP) {
		for (X = *SP; X->next; X = X->next);
		X->next = S;
		S->prev = X;
	} else {
		S->prev = 0;
		*SP = S;
	}
}

char *
Unitid(unit * u)
{
	static char buf[NAMESIZE];
	static char un[30];
	strncpy(un, u->name, 25);
	un[25] = 0;
	sprintf(buf, "%s (%s)", un, unitid(u));
	return buf;
}

char *
Buildingid(building * b)
{
	static char buf[35];
	sprintf(buf, "\002%s (%s), Größe %d",
			buildingtype(b, b->size, NULL),
			buildingid(b), b->size);
	return buf;
}

char *
Shipid(ship * sh)
{
	static char buf[30];
	sprintf(buf, "\023%s (%s)", sh->type->name[0], shipid(sh));
	return buf;
}

char *
BuildingName(building * b)
{
	static char buf[35];
	sprintf(buf, "%s (%s)",
			buildingtype(b, b->size, NULL), buildingid(b));
	return buf;
}

int
map_input(WINDOW * win, int x, int y, const char *text, int mn, int mx, int pre)
{
	char lbuf[10];
	int val, ch, cx, nw = 0;
	if (!win) {
		win = openwin(50, 3, 0);
		nw = y = 1;
		x = 2;
	} else
		wrefresh(win);
	do {
		sprintf(lbuf, "%d", pre);
		wmove(win, y, x);
		curs_set(1);
		wprintw(win, (NCURSES_CONST char*)"%s (%d..%d): %d", text, mn, mx, pre);
		wrefresh(win);
		cx = getcurx(win) - strlen(lbuf);
		val = strlen(lbuf);
		do {
			ch = getch();
			if (ch==8 || ch == KEY_BACKSPACE || ch == KEY_LEFT) {
				if (val == 0)
					beep();
				else {
					val--;
					wmove(win, y, val + cx);
					waddch(win, ' ');
					wmove(win, y, val + cx);
					wrefresh(win);
				}
			} else if (ch == 10 || ch == 13) {
				curs_set(0);
			} else if ((ch == '-' && val == 0) || (ch >= '0' && ch <= '9' && val < 10)) {
				waddch(win, ch);
				lbuf[val] = (char) ch;
				val++;
			} else
				beep();
			wrefresh(win);
		} while (!(ch == 10 || ch == 13));
		lbuf[val] = 0;
		val = atoi(lbuf);
		if (val < mn || val > mx)
			beep();
	} while (val < mn || val > mx);
	if (nw)
		delwin(win);
	return val;
}

void
warnung(WINDOW * win, const char *text)
{
	if (!win) {
		win = openwin(strlen(text) + 4, 3, "< WARNUNG >");
		wmove(win, 1, 2);
	}
	wprintw(win, (NCURSES_CONST char*)"%s", text);
	beep();
	wrefresh(win);
	getch();
	delwin(win);
}

boolean
yes_no(WINDOW * win, const char *text, const char def)
{
	int ch;
	boolean mywin = false;
	if (!win) {
		win = openwin(strlen(text) + 10, 3, 0);
		wmove(win, 1, 2);
		mywin = true;
	}
	wprintw(win, (NCURSES_CONST char*)"%s (%c/%c)", text, def == 'j' ? 'J' : 'j', def == 'n' ? 'N' : 'n');
	wrefresh(win);
	ch = getch();
	if (mywin) {
		delwin(win);
	}
	if (ch == 10 || ch == 13)
		ch = def;
	return (boolean)((ch == 'j' || ch == 'J' || ch == 'y' || ch == 'Y')?true:false);
}

char *
my_input(WINDOW * win, int x, int y, const char *text, const char *def)
{
	static char lbuf[INPUT_BUFSIZE+1];
	int val, ch, p, nw = 0;

	if (!win) {
		win = openwin(SX - 10, 3, 0);
		y = nw = 1;
		x = 2;
	}

	wmove(win, y, x);
	wAddstr(text);
	if(def) {
		strcpy(lbuf, def);
		wAddstr(lbuf);
		p = x + strlen(text);
		val = strlen(lbuf);
		wmove(win, y, p + val);
	} else {
		p = x + strlen(text);
		wmove(win, y, p);
		val = 0;
	}
	wrefresh(win);
	curs_set(1);
	wcursyncup(win);

	do {
		ch = getch();
		if (ch == KEY_BACKSPACE || ch == KEY_LEFT) {
			if (val == 0)
				beep();
			else {
				val--;
				wmove(win, y, val + p);
				waddch(win, ' ');
				wmove(win, y, val + p);
				wrefresh(win);
			}
		} else if (ch == '\n') {
			curs_set(0);
		} if(val >= INPUT_BUFSIZE) {
			beep();
		} else if (isprint(ch)) {
			waddch(win, ch);
			lbuf[val] = (char) ch;
			val++;
		} else
			beep();
		wrefresh(win);
	} while (!(ch == '\n'));
	if (nw)
		delwin(win);
	curs_set(0);
	lbuf[val] = 0;
	return lbuf;
}

void
insert_selection(selection ** p_sel, selection * prev, char * str, void * payload)
{
	selection * sel = calloc(sizeof(selection), 1);
	sel->str = str;
	sel->data = payload;
	if (*p_sel) {
		selection * s;
		sel->next = *p_sel;
		sel->prev = sel->next->prev;
		sel->next->prev=sel;
		if (sel->prev) {
			sel->prev->next = sel;
			sel->index=sel->prev->index+1;
		}
		for (s=sel->next;s;s=s->next) {
			s->index = s->prev->index+1;
		}
	} else {
		*p_sel = sel;
		sel->prev = prev;
	}
}

selection **
push_selection(selection ** p_sel, char * str, void * payload)
{
	selection * sel = calloc(sizeof(selection), 1);
	selection * prev = NULL;
	sel->str = str;
	sel->data = payload;
	while (*p_sel) {
		prev = *p_sel;
		p_sel=&prev->next;
	}
	*p_sel = sel;
	if (prev) {
		sel->prev = prev;
		sel->index = prev->index+1;
	}
	return p_sel;
}

selection *
do_selection(selection * sel, const char * title, void (*perform)(selection *, void *), void * data)
{
	WINDOW * wn;
	boolean update = true;
	selection *s;
	selection *top = sel;
	selection *current = top;
	int i;
	int height=0, width=strlen(title)+8;
	for (s=sel;s;s=s->next) {
		if (strlen(s->str)>(unsigned)width) width = strlen(s->str);
		++height;
	}
	if (height==0 || width==0) return NULL;
	if (width+3>SX) width=SX-4;
	if (height+2>SY) height=SY-2;

	wn = newwin(height+2, width+4, (SY - height - 2) / 2, (SX - width - 4) / 2);

	for (;;) {
		int input;
		if (update) {
			wclear(wn);
			for (s=top;s!=NULL && top->index+height!=s->index;s=s->next) {
				i = s->index-top->index;
				wmove(wn, i + 1, 4);
				waddnstr(wn, s->str, -1);
			}
			wborder(wn, '|', '|', '-', '-', '+', '+', '+', '+');
			wmove(wn, 0, 2);
			sprintf(buf, "[ %s ]", title);
			waddnstr(wn, buf, width);
			update = false;
		}
		i = current->index-top->index;
		wmove(wn, i + 1, 2);
		waddstr(wn, "->");
		wmove(wn, i + 1, 4);
		wattron(wn, A_BOLD);
		waddnstr(wn, current->str, width);
		wattroff(wn, A_BOLD);

		wrefresh(wn);

		input = getch();

		wmove(wn, i + 1, 2);
		waddstr(wn, "  ");
		wmove(wn, i + 1, 4);
		waddnstr(wn, current->str, width);

		switch (input) {
		case KEY_DOWN:
			if (current->next) {
				current = current->next;
				if (current->index-height>=top->index) {
					top=current;
					update = true;
				}
			}
			break;
		case KEY_UP:
			if (current->prev) {
				if (current==top) {
					top = sel;
					while (top->index+height<current->index) top=top->next;
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
			if (perform) perform(current, data);
			else {
				delwin(wn);
				return current;
			}
			break;
		}
	}
}
