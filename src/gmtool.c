#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <curses.h>
#ifdef HAVE_LUA
#include <lua.h>
#include "console.h"
#endif
#include "gmtool.h"

#include <modules/autoseed.h>

#include "kernel/building.h"
#include "kernel/config.h"
#include "kernel/faction.h"
#include "kernel/direction.h"
#include "kernel/item.h"
#include "kernel/plane.h"
#include "kernel/region.h"
#include "kernel/resources.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"
#include "kernel/save.h"
#include "kernel/ship.h"
#include "kernel/terrain.h"

#include <kernel/attrib.h>
#include <util/base36.h>
#include <util/lists.h>
#include <util/macros.h>
#include "util/path.h"
#include "util/rng.h"

#include "gmtool_structs.h"
#include "listbox.h"
#include "teleport.h"

#include <selist.h>
#include <stb_ds.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int g_quit;
int force_color = 0;
newfaction * new_players = NULL;

state *current_state = NULL;

#define IFL_SHIPS     (1<<0)
#define IFL_UNITS     (1<<1)
#define IFL_FACTIONS  (1<<2)
#define IFL_BUILDINGS (1<<3)

static WINDOW *hstatus;

int umvwprintw(WINDOW *win, int y, int x, const char *format, ...) {
    char buffer[128];
    va_list args;

    va_start(args, format);
    memset(buffer, 0, sizeof(buffer));
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);

    return mvwaddstr(win, y, x, buffer);
}

int umvwaddnstr(WINDOW *w, int y, int x, const char * str, int len) {
    (void)len;
    return mvwaddstr(w, y, x, str);
}

static void init_curses(void)
{
    initscr();

    if (has_colors() || force_color) {
        int fg, bg;
        short bcol = COLOR_BLACK;
        short hcol = COLOR_MAGENTA;
        start_color();
        /* looks crap on putty with TERM=linux */
        if (can_change_color()) {
            init_color(COLOR_YELLOW, 1000, 1000, 0);
            init_color(COLOR_CYAN, 0, 1000, 1000);
        }
        for (fg = 0; fg != 8; ++fg) {
            for (bg = 0; bg != 2; ++bg) {
                init_pair((short)(fg + 8 * bg), (short)fg, (short)(bg ? hcol : bcol));
            }
        }

        attrset(COLOR_PAIR(COLOR_BLACK));
        bkgd(' ' | COLOR_PAIR(COLOR_BLACK));
        bkgdset(' ' | COLOR_PAIR(COLOR_BLACK));
    }

    keypad(stdscr, TRUE);         /* enable keyboard mapping */
    meta(stdscr, TRUE);
    nonl();                       /* tell curses not to do NL->CR/NL on output */
    cbreak();                     /* take input chars one at a time, no wait for \n */
    noecho();                     /* don't echo input */
    scrollok(stdscr, FALSE);
    refresh();
}

void cnormalize(const coordinate * c, int *x, int *y)
{
    *x = c->x;
    *y = c->y;
    pnormalize(x, y, c->pl);
}

map_region *mr_get(const view * vi, int xofs, int yofs)
{
    return vi->regions + xofs + yofs * vi->size.width;
}

static point *coor2point(const coordinate * c, point * p)
{
    assert(c && p);
    p->x = c->x * TWIDTH + c->y * TWIDTH / 2; /*-V537 */
    p->y = c->y * THEIGHT;
    return p;
}

static window *wnd_first, *wnd_last;

static window *win_create(WINDOW * hwin)
{
    window *wnd = calloc(1, sizeof(window));
    wnd->handle = hwin;
    if (wnd_first != NULL) {
        wnd->next = wnd_first;
        wnd_first->prev = wnd;
        wnd_first = wnd;
    }
    else {
        wnd_first = wnd;
        wnd_last = wnd;
    }
    return wnd;
}

static void untag_all_regions(selection *s) {
    int i;
    tag **tp;
    tag *t;
    for(i=0; i < MAXTHASH; ++i) {
        tp = &s->tags[i];
        while(*tp) {
            t = *tp;
            *tp = t->nexthash;
            free(t);
        }
    }
}

static void untag_region(selection * s, int nx, int ny)
{
    unsigned int key = ((nx << 12) ^ ny);
    tag **tp = &s->tags[key & (MAXTHASH - 1)];
    tag *t = NULL;
    while (*tp) {
        t = *tp;
        if (t->coord.x == nx && t->coord.y == ny)
            break;
        tp = &t->nexthash;
    }
    if (!*tp)
        return;
    *tp = t->nexthash;
    free(t);
    return;
}

static void tag_region(selection * s, int nx, int ny)
{
    unsigned int key = ((nx << 12) ^ ny);
    tag **tp = &s->tags[key & (MAXTHASH - 1)];
    while (*tp) {
        tag *t = *tp;
        if (t->coord.x == nx && t->coord.y == ny)
            return;
        tp = &t->nexthash;
    }
    *tp = calloc(1, sizeof(tag));
    (*tp)->coord.x = nx;
    (*tp)->coord.y = ny;
    (*tp)->coord.pl = findplane(nx, ny);
    return;
}

static int tagged_region(selection * s, int nx, int ny)
{
    unsigned int key = ((nx << 12) ^ ny);
    tag **tp = &s->tags[key & (MAXTHASH - 1)];
    while (*tp) {
        tag *t = *tp;
        if (t->coord.x == nx && t->coord.y == ny)
            return 1;
        tp = &t->nexthash;
    }
    return 0;
}

static chtype mr_tile(const map_region * mr, int highlight)
{
    int hl = 8 * highlight;
    if (mr != NULL && mr->r != NULL) {
        const region *r = mr->r;
        switch (r->terrain->_name[0]) {
        case 'o':
            return '.' | COLOR_PAIR(hl + COLOR_CYAN) | A_BOLD; /*-V525 */
        case 'd':
            return 'D' | COLOR_PAIR(hl + COLOR_YELLOW) | A_BOLD;
        case 't':
            return '%' | COLOR_PAIR(hl + COLOR_YELLOW) | A_BOLD;
        case 'f':
            if (r->terrain->_name[1] == 'o') {      /* fog */
                return '.' | COLOR_PAIR(hl + COLOR_YELLOW) | A_NORMAL;
            }
            else if (r->terrain->_name[1] == 'i') {       /* firewall */
                return '%' | COLOR_PAIR(hl + COLOR_RED) | A_BOLD;
            }
            break;
        case 'h':
            return 'H' | COLOR_PAIR(hl + COLOR_YELLOW) | A_NORMAL;
        case 'm':
            return '^' | COLOR_PAIR(hl + COLOR_WHITE) | A_NORMAL;
        case 'p':
            if (r->terrain->_name[1] == 'l') {      /* plain */
                if (r_isforest(r))
                    return '#' | COLOR_PAIR(hl + COLOR_GREEN) | A_NORMAL;
                return '+' | COLOR_PAIR(hl + COLOR_GREEN) | A_BOLD;
            }
            else if (r->terrain->_name[1] == 'a') {       /* packice */
                return ':' | COLOR_PAIR(hl + COLOR_WHITE) | A_BOLD;
            }
            break;
        case 'g':
            return '*' | COLOR_PAIR(hl + COLOR_WHITE) | A_BOLD;
        case 's':
            return 'S' | COLOR_PAIR(hl + COLOR_MAGENTA) | A_NORMAL;
        }
        return r->terrain->_name[0] | COLOR_PAIR(hl + COLOR_RED);
    }
    return ' ' | COLOR_PAIR(hl + COLOR_WHITE);
}

typedef chtype (*draw_fun)(const map_region*, int);

static chtype draw_terrain(const map_region* mr, int highlight)
{
    return mr_tile(mr, highlight);
}

static chtype draw_luxury(const map_region* mr, int highlight)
{
    int hl = 8 * highlight;
    if (mr && mr->r) {
        const item_type* it_lux = r_luxury(mr->r);
        if (it_lux) {
            return it_lux->rtype->_name[0] | COLOR_PAIR(hl + COLOR_WHITE);
        }
    }
    return mr_tile(mr, hl);
}

static chtype draw_tile(window *win, const map_region* mr, int highlight)
{
    if (win->data) {
        draw_fun foo = (draw_fun)win->data;
        return foo(mr, highlight);
    }
    return mr_tile(mr, highlight);
}

static void paint_map(window * wnd, const state * st)
{
    WINDOW *win = wnd->handle;
    int lines = getmaxy(win);
    int cols = getmaxx(win);
    int vx, vy;

    assert(st);
    if (!st) return;
    lines = lines / THEIGHT;
    cols = cols / TWIDTH;
    for (vy = 0; vy != lines; ++vy) {
        int yp = (lines - vy - 1) * THEIGHT;
        for (vx = 0; vx != cols; ++vx) {
            map_region *mr = mr_get(&st->display, vx, vy);
            int xp = vx * TWIDTH + (vy & 1) * TWIDTH / 2;
            int nx, ny;
            if (mr) {
                int attr = 0;
                int hl = 0;
                chtype tile;
                cnormalize(&mr->coord, &nx, &ny);
                if (tagged_region(st->selected, nx, ny)) {
                    attr |= A_REVERSE;
                }
                if (mr->r && (mr->r->flags & RF_MAPPER_HIGHLIGHT))
                    hl = 1;
                tile = draw_tile(wnd, mr, hl);
                mvwaddch(win, yp, xp, tile | attr);
            }
        }
    }
}

map_region *cursor_region(const view * v, const coordinate * c)
{
    if (c) {
        int cx, cy;
        coordinate relpos;

        relpos.x = c->x - v->topleft.x;
        relpos.y = c->y - v->topleft.y;
        cy = relpos.y;
        cx = relpos.x + cy / 2;
        return mr_get(v, cx, cy);
    }
    return NULL;
}

static void
draw_cursor(window * wnd, selection * s, const view * v, const coordinate * c,
    int show)
{
    WINDOW* win = wnd->handle;
    int lines = getmaxy(win) / THEIGHT;
    int xp, yp, nx, ny;
    int attr = 0;
    map_region *mr = cursor_region(v, c);
    coordinate relpos;
    int cx, cy;

    if (!mr)
        return;

    relpos.x = c->x - v->topleft.x;
    relpos.y = c->y - v->topleft.y;
    cy = relpos.y;
    cx = relpos.x + cy / 2;

    yp = (lines - cy - 1) * THEIGHT;
    xp = cx * TWIDTH + (cy & 1) * TWIDTH / 2;
    cnormalize(&mr->coord, &nx, &ny);
    if (s && tagged_region(s, nx, ny))
        attr = A_REVERSE;
    if (mr->r) {
        int hl = 0;
        if (mr->r->flags & RF_MAPPER_HIGHLIGHT)
            hl = 1;
        mvwaddch(win, yp, xp, draw_tile(wnd, mr, hl) | attr);
    }
    else
        mvwaddch(win, yp, xp, ' ' | attr | COLOR_PAIR(COLOR_YELLOW));
    if (show) {
        attr = A_BOLD;
        mvwaddch(win, yp, xp - 1, '<' | attr | COLOR_PAIR(COLOR_YELLOW));
        mvwaddch(win, yp, xp + 1, '>' | attr | COLOR_PAIR(COLOR_YELLOW));
    }
    else {
        attr = A_NORMAL;
        mvwaddch(win, yp, xp - 1, ' ' | attr | COLOR_PAIR(COLOR_WHITE));
        mvwaddch(win, yp, xp + 1, ' ' | attr | COLOR_PAIR(COLOR_WHITE));
    }
    wmove(win, yp, xp);
    wnoutrefresh(win);
}

static void paint_status(window * wnd, const state * st)
{
    WINDOW *win = wnd->handle;
    const char *name = "";
    int nx, ny, uid = 0;
    const char *terrain = "----";
    map_region *mr = cursor_region(&st->display, &st->cursor);
    if (mr && mr->r) {
        uid = mr->r->uid;
        if (mr->r->land) {
            name = (const char *)mr->r->land->name;
        }
        else {
            name = mr->r->terrain->_name;
        }
        terrain = mr->r->terrain->_name;
    }
    cnormalize(&st->cursor, &nx, &ny);
    umvwprintw(win, 0, 0, "%4d %4d | %.4s | %.20s (%d)", nx, ny, terrain, name,
        uid);
    wclrtoeol(win);
}

static bool handle_info_region(window * wnd, state * st, int c)
{
    return false;
}

static void paint_info_region(window * wnd, const state * st)
{
    WINDOW *win = wnd->handle;
    int size = getmaxx(win) - 2;
    int maxline = getmaxy(win) - 2;
    map_region *mr = cursor_region(&st->display, &st->cursor);

    UNUSED_ARG(st);
    werase(win);
    if (mr && mr->r) {
        int line = 0;
        const region *r = mr->r;
        if (r->land) {
            umvwaddnstr(win, line++, 1, (char *)r->land->name, size);
        }
        else {
            umvwaddnstr(win, line++, 1, r->terrain->_name, size);
        }
        line++;
        umvwprintw(win, line++, 1, "%s, age %d", r->terrain->_name, r->age);
        if (r->land) {
            int iron = region_getresource_level(r, get_resourcetype(R_IRON));
            int stone = region_getresource_level(r, get_resourcetype(R_STONE));
            mvwprintw(win, line++, 1, "$:%8d  P:%8d", rmoney(r), rpeasants(r));
            mvwprintw(win, line++, 1, "S:%8d  I:%8d", stone, iron);
            mvwprintw(win, line++, 1, "H:%8d  %s:%8d", rhorses(r),
                (r->flags & RF_MALLORN) ? "M" : "T",
                r->land->trees[1] + r->land->trees[2]);
        }
        line++;
        if (r->units && (st->info_flags & IFL_FACTIONS)) {
            unit *u;
            wattron(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
            mvwaddnstr(win, line++, 1, "* factions:", size - 5);
            wattroff(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
            for (u = r->units; u && line < maxline; u = u->next) {
                if (!fval(u->faction, FFL_MARK)) {
                    mvwprintw(win, line, 1, "%.4s ", itoa36(u->faction->no));
                    umvwaddnstr(win, line++, 6, (char *)u->faction->name, size - 5);
                    fset(u->faction, FFL_MARK);
                }
            }
            for (u = r->units; u && line < maxline; u = u->next) {
                freset(u->faction, FFL_MARK);
            }
        }
        if (r->units && (st->info_flags & IFL_UNITS)) {
            unit *u;
            wattron(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
            mvwaddnstr(win, line++, 1, "* units:", size - 5);
            wattroff(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
            for (u = r->units; u && line < maxline; u = u->next) {
                mvwprintw(win, line, 1, "%.4s ", itoa36(u->no));
                umvwaddnstr(win, line++, 6, unit_getname(u), size - 5);
            }
        }
        if (r->ships && (st->info_flags & IFL_SHIPS)) {
            ship *sh;
            wattron(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
            mvwaddnstr(win, line++, 1, "* ships:", size - 5);
            wattroff(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
            for (sh = r->ships; sh && line < maxline; sh = sh->next) {
                mvwprintw(win, line, 1, "%.4s ", itoa36(sh->no));
                umvwaddnstr(win, line++, 6, (char *)sh->type->_name, size - 5);
            }
        }
    }
    box(win, 0, 0);
}

static void(*paint_info) (struct window * wnd, const struct state * st);

static void paint_info_default(window * wnd, const state * st)
{
    if (paint_info)
        paint_info(wnd, st);
    else
        paint_info_region(wnd, st);
}

void set_info_function(void(*callback) (struct window *, const struct state *))
{
    paint_info = callback;
}

static char *askstring(WINDOW * win, const char *q, char *buffer, size_t size)
{
    werase(win);
    mvwaddstr(win, 0, 0, (char *)q);
    wmove(win, 0, (int)(strlen(q) + 1));
    echo();
    wgetnstr(win, buffer, (int)size);
    noecho();
    return buffer;
}

static void statusline(WINDOW * win, const char *str)
{
    mvwaddstr(win, 0, 0, (char *)str);
    wclrtoeol(win);
    wnoutrefresh(win);
}

static void reset_region(region *r) {
    unit **up = &r->units;
    bool players = false;

    r->flags = 0;
    a_removeall(&r->attribs, NULL);
    while (*up) {
        unit *u = *up;
        if (IS_MONSTERS(u->faction)) {
            remove_unit(up, u);
        }
        else {
            players = true;
            up = &u->next;
        }
    }
    if (!players) {
        while (r->ships) {
            remove_ship(&r->ships, r->ships);
        }
        while (r->buildings) {
            remove_building(&r->buildings, r->buildings);
        }
        if (r->land) {
            init_region(r);
            terraform_resources(r);
        }
    }
}

static region * state_region(state *st) {
    int nx = st->cursor.x;
    int ny = st->cursor.y;
    pnormalize(&nx, &ny, st->cursor.pl);
    return findregion(nx, ny);
}

static void reset_area_cb(void *arg) {
    region *r = (region *)arg;
    r->age = 0;
    freset(r, RF_MARK);
}

static void reset_area(state *st) {
    region * r = state_region(st);
    if (r != NULL) {
        selist * ql = NULL;
        int qi = 0, qlen = 0;
        fset(r, RF_MARK);
        selist_insert(&ql, qlen++, r);
        while (qi != qlen) {
            int i;
            region *adj[MAXDIRECTIONS];
            r = selist_get(ql, qi++);
            get_neighbours(r, adj);
            for (i = 0; i != MAXDIRECTIONS; ++i) {
                region *rn = adj[i];
                if (rn && !fval(rn, RF_MARK)) {
                    if ((rn->terrain->flags & FORBIDDEN_REGION) == 0) {
                        fset(rn, RF_MARK);
                        selist_insert(&ql, qlen++, rn);
                    }
                }
            }
        }
        selist_foreach(ql, reset_area_cb);
        selist_free(ql);
    }
}

static void reset_cursor(state *st) {
    region * r = state_region(st);
    if (r != NULL) {
        reset_region(r);
    }
}

static void reset_rect(state *st) {
    int x, y, bs = 3;
    for (x=0;x!=bs;++x) {
        for (y = 0; y != bs; ++y) {
            region *r;
            int nx = st->cursor.x + x;
            int ny = st->cursor.y + y;
            pnormalize(&nx, &ny, st->cursor.pl);
            if ((r = findregion(nx, ny)) != NULL) {
                reset_region(r);
            }
        }
    }
}

static void terraform_at(coordinate * c, const terrain_type * terrain)
{
    if (terrain != NULL) {
        region *r;
        int nx = c->x, ny = c->y;
        pnormalize(&nx, &ny, c->pl);
        r = findregion(nx, ny);
        if (r == NULL) {
            r = new_region(nx, ny, c->pl, 0);
        }
        if (!(r->units && r->land && !fval(terrain, LAND_REGION))) {
            terraform_region(r, terrain);
        }
    }
}

static void selection_walk(selection * selected, void(*callback)(region *, void *), void *udata) {
    int i;

    for (i = 0; i != MAXTHASH; ++i) {
        tag **tp = &selected->tags[i];
        while (*tp) {
            region *r;
            tag *t = *tp;
            int nx = t->coord.x, ny = t->coord.y;
            plane *pl = t->coord.pl;

            pnormalize(&nx, &ny, pl);
            r = findregion(nx, ny);
            if (r != NULL) {
                callback(r, udata);
            }
            tp = &t->nexthash;
        }
    }
}

static void reset_levels_cb(region *r, void *udata) {
    ptrdiff_t i, len = arrlen(r->resources);
    UNUSED_ARG(udata);
    for (i = 0; i != len; ++i) {
        struct rawmaterial* res = r->resources + i;
        if (res->level > 3) {
            res->level = 1;
        }
    }
}

/**
 * BUG 2506: reset drained mountains to level 1
 */
static void
fix_selection(selection * selected)
{
    selection_walk(selected, reset_levels_cb, NULL);
}

static void
reset_selection(selection * selected)
{
    int i;

    for (i = 0; i != MAXTHASH; ++i) {
        tag **tp = &selected->tags[i];
        while (*tp) {
            region *r;
            tag *t = *tp;
            int nx = t->coord.x, ny = t->coord.y;
            plane *pl = t->coord.pl;

            pnormalize(&nx, &ny, pl);
            r = findregion(nx, ny);
            if (r != NULL) {
                reset_region(r);
            }
            tp = &t->nexthash;
        }
    }
}

static void
terraform_selection(selection * selected, const terrain_type * terrain)
{
    int i;

    if (terrain == NULL)
        return;
    for (i = 0; i != MAXTHASH; ++i) {
        tag **tp = &selected->tags[i];
        while (*tp) {
            region *r;
            tag *t = *tp;
            int nx = t->coord.x, ny = t->coord.y;
            plane *pl = t->coord.pl;

            pnormalize(&nx, &ny, pl);
            r = findregion(nx, ny);
            if (r == NULL) {
                r = new_region(nx, ny, pl, 0);
            }
            if (!(r->units && r->land && !fval(terrain, LAND_REGION))) {
                terraform_region(r, terrain);
            }
            tp = &t->nexthash;
        }
    }
}

static faction *select_faction(state * st)
{
    list_selection *ilist = NULL, **iinsert;
    list_selection *selected = NULL;
    faction *f = factions;

    if (!f)
        return NULL;
    iinsert = &ilist;

    while (f) {
        char buffer[32];
        sprintf(buffer, "%.4s %.26s", itoa36(f->no), f->name);
        insert_selection(iinsert, NULL, buffer, (void *)f);
        f = f->next;
    }
    selected = do_selection(ilist, "Select Faction", NULL, NULL);
    st->wnd_info->update |= 1;
    st->wnd_map->update |= 3;
    st->wnd_status->update |= 1;

    if (selected == NULL)
        return NULL;
    return (faction *)selected->data;
}

static const terrain_type *select_terrain(state * st,
    const terrain_type * default_terrain)
{
    list_selection *ilist = NULL, **iinsert;
    list_selection *selected = NULL;
    const terrain_type *terrain = terrains();

    if (!terrain)
        return NULL;
    iinsert = &ilist;

    while (terrain) {
        insert_selection(iinsert, NULL, terrain->_name, (void *)terrain);
        terrain = terrain->next;
    }
    selected = do_selection(ilist, "Terrain", NULL, NULL);
    st->wnd_info->update |= 1;
    st->wnd_map->update |= 3;
    st->wnd_status->update |= 1;

    if (selected == NULL)
        return NULL;
    return (const terrain_type *)selected->data;
}

static coordinate *region2coord(const region * r, coordinate * c)
{
    c->x = r->x;
    c->y = r->y;
    c->pl = rplane(r);
    return c;
}
#ifdef PDCURSES
#define FAST_UP CTL_UP
#define FAST_DOWN CTL_DOWN
#define FAST_LEFT CTL_LEFT
#define FAST_RIGHT CTL_RIGHT
#else
#define FAST_UP KEY_PPAGE
#define FAST_DOWN KEY_NPAGE
#define FAST_LEFT KEY_SLEFT
#define FAST_RIGHT KEY_SRIGHT
#endif

void highlight_region(region * r, int toggle)
{
    if (r != NULL) {
        if (toggle)
            r->flags |= RF_MAPPER_HIGHLIGHT;
        else
            r->flags &= ~RF_MAPPER_HIGHLIGHT;
    }
}

void select_coordinate(struct selection *selected, int nx, int ny, bool toggle)
{
    if (toggle)
        tag_region(selected, nx, ny);
    else
        untag_region(selected, nx, ny);
}

enum select_t { MODE_MARK, MODE_SELECT, MODE_UNMARK, MODE_UNSELECT };

static void select_island(state *st, int selectmode)
{
    region *r;
    int nx = st->cursor.x;
    int ny = st->cursor.y;

    pnormalize(&nx, &ny, st->cursor.pl);
    r = findregion(nx, ny);
    if (r && r->land) {
        selist *ql, *stack = NULL;
        int qi = 0;

        selist_push(&stack, r);
        for (ql = stack, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            region *r = (region *)selist_get(ql, qi);
            region *rnext[MAXDIRECTIONS];
            int i;

            fset(r, RF_MARK);
            if (selectmode & MODE_SELECT) {
                select_coordinate(st->selected, r->x, r->y,
                    selectmode == MODE_SELECT);
            }
            else {
                highlight_region(r, selectmode == MODE_MARK);
            }
            get_neighbours(r, rnext);
            for (i = 0; i != MAXDIRECTIONS; ++i) {
                region *rn = rnext[i];
                if (rn && rn->land && !fval(rn, RF_MARK)) {
                    selist_push(&stack, rn);
                }
            }
        }

        for (ql = stack, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            region *r = (region *)selist_get(ql, qi);
            freset(r, RF_MARK);
        }
        selist_free(stack);
    }
}

static void select_regions(state * st, int selectmode)
{
    char sbuffer[80];
    int findmode;
    const char *statustext[] = {
        "mark-", "select-", "unmark-", "deselect-"
    };
    const char *status = statustext[selectmode];
    statusline(st->wnd_status->handle, status);
    doupdate();
    findmode = getch();
    if (findmode == 'n') {        /* none */
        sprintf(sbuffer, "%snone", status);
        statusline(st->wnd_status->handle, sbuffer);
        if (selectmode & MODE_SELECT) {
            int i;
            for (i = 0; i != MAXTHASH; ++i) {
                tag **tp = &st->selected->tags[i];
                while (*tp) {
                    tag *t = *tp;
                    *tp = t->nexthash;
                    free(t);
                }
            }
        }
        else {
            region *r;
            for (r = regions; r; r = r->next) {
                r->flags &= ~RF_MAPPER_HIGHLIGHT;
            }
        }
    }
    else if (findmode == 'v') {
        region *r;
        /* fresh virgin regions */
        sprintf(sbuffer, "%svirgin", status);
        statusline(st->wnd_status->handle, sbuffer);
        for (r = regions; r; r = r->next) {
            if (r->age == 0) {
                if (selectmode & MODE_SELECT) {
                    select_coordinate(st->selected, r->x, r->y,
                        selectmode == MODE_SELECT);
                }
                else {
                    highlight_region(r, selectmode == MODE_MARK);
                }
            }
        }
    }
    else if (findmode == 'c') {
        region *r;
        sprintf(sbuffer, "%schaos", status);
        statusline(st->wnd_status->handle, sbuffer);
        for (r = regions; r; r = r->next) {
            if (fval(r, RF_CHAOTIC)) {
                if (selectmode & MODE_SELECT) {
                    select_coordinate(st->selected, r->x, r->y,
                        selectmode == MODE_SELECT);
                }
                else {
                    highlight_region(r, selectmode == MODE_MARK);
                }
            }
        }
    }
    else if (findmode == 'm') {
        region *r;
        sprintf(sbuffer, "%smonsters", status);
        statusline(st->wnd_status->handle, sbuffer);
        for (r = regions; r; r = r->next) {
            unit *u = r->units;
            for (; u; u = u->next) {
                if (fval(u->faction, FFL_NPC) != 0)
                    break;
            }
            if (u) {
                if (selectmode & MODE_SELECT) {
                    select_coordinate(st->selected, r->x, r->y,
                        selectmode == MODE_SELECT);
                }
                else {
                    highlight_region(r, selectmode == MODE_MARK);
                }
            }
        }
    }
    else if (findmode == 'p') {
        region *r;
        sprintf(sbuffer, "%splayers", status);
        statusline(st->wnd_status->handle, sbuffer);
        for (r = regions; r; r = r->next) {
            unit *u = r->units;
            for (; u; u = u->next) {
                if (fval(u->faction, FFL_NPC) == 0)
                    break;
            }
            if (u) {
                if (selectmode & MODE_SELECT) {
                    select_coordinate(st->selected, r->x, r->y,
                        selectmode == MODE_SELECT);
                }
                else {
                    highlight_region(r, selectmode == MODE_MARK);
                }
            }
        }
    }
    else if (findmode == 'u') {
        region *r;
        sprintf(sbuffer, "%sunits", status);
        statusline(st->wnd_status->handle, sbuffer);
        for (r = regions; r; r = r->next) {
            if (r->units) {
                if (selectmode & MODE_SELECT) {
                    select_coordinate(st->selected, r->x, r->y,
                        selectmode == MODE_SELECT);
                }
                else {
                    highlight_region(r, selectmode == MODE_MARK);
                }
            }
        }
    }
    else if (findmode == 's') {
        region *r;
        sprintf(sbuffer, "%sships", status);
        statusline(st->wnd_status->handle, sbuffer);
        for (r = regions; r; r = r->next) {
            if (r->ships) {
                if (selectmode & MODE_SELECT) {
                    select_coordinate(st->selected, r->x, r->y,
                        selectmode == MODE_SELECT);
                }
                else {
                    highlight_region(r, selectmode == MODE_MARK);
                }
            }
        }
    }
    else if (findmode == 'f') {
        char fbuffer[12];
        sprintf(sbuffer, "%sfaction:", status);
        askstring(st->wnd_status->handle, sbuffer, fbuffer, 12);
        if (fbuffer[0]) {
            faction *f = findfaction(atoi36(fbuffer));

            if (f != NULL) {
                unit *u;

                sprintf(sbuffer, "%sfaction: %s", status, itoa36(f->no));
                statusline(st->wnd_status->handle, sbuffer);
                for (u = f->units; u; u = u->nextF) {
                    region *r = u->region;
                    if (selectmode & MODE_SELECT) {
                        select_coordinate(st->selected, r->x, r->y,
                            selectmode == MODE_SELECT);
                    }
                    else {
                        highlight_region(r, selectmode == MODE_MARK);
                    }
                }
            }
            else {
                statusline(st->wnd_status->handle, "faction not found.");
                beep();
                return;
            }
        }
    }
    else if (findmode == 'i') {
        sprintf(sbuffer, "%swand: ", status);
        statusline(st->wnd_status->handle, sbuffer);
        select_island(st, selectmode);
    }
    else if (findmode == 't') {
        const struct terrain_type *terrain;
        sprintf(sbuffer, "%sterrain: ", status);
        statusline(st->wnd_status->handle, sbuffer);
        terrain = select_terrain(st, NULL);
        if (terrain != NULL) {
            region *r;
            sprintf(sbuffer, "%sterrain: %s", status, terrain->_name);
            statusline(st->wnd_status->handle, sbuffer);
            for (r = regions; r; r = r->next) {
                if (r->terrain == terrain) {
                    if (selectmode & MODE_SELECT) {
                        select_coordinate(st->selected, r->x, r->y,
                            selectmode == MODE_SELECT);
                    }
                    else {
                        highlight_region(r, selectmode == MODE_MARK);
                    }
                }
            }
        }
    }
    else {
        statusline(st->wnd_status->handle, "unknown command.");
        beep();
        return;
    }
    st->wnd_info->update |= 3;
    st->wnd_status->update |= 3;
    st->wnd_map->update |= 3;
}

static void loaddata(state *st) {
    char datafile[PATH_MAX];

    askstring(st->wnd_status->handle, "load from:", datafile, sizeof(datafile));
    if (strlen(datafile) > 0) {
        readgame(datafile);
        st->modified = 0;
    }
}

static void savedata(state *st) {
    char datafile[PATH_MAX];

    askstring(st->wnd_status->handle, "save as:", datafile, sizeof(datafile));
    if (strlen(datafile) > 0) {
        remove_empty_units();
        writegame(datafile);
        st->modified = 0;
    }
}

static bool seed_player(state *st, const newfaction *player) {
    if (player) {
        region *r;
        int nx = st->cursor.x;
        int ny = st->cursor.y;

        pnormalize(&nx, &ny, st->cursor.pl);
        r = findregion(nx, ny);
        if (r) {
            if (r->land) {
                faction *f = addfaction(player->email, player->password,
                    player->race, player->lang);
                addplayer(r, f);
                return true;
            }
        }
    }
    return false;
}

static bool confirm(WINDOW * win, const char *q) {
    int ch;
    werase(win);
    mvwaddstr(win, 0, 0, (char *)q);
    wmove(win, 0, (int)(strlen(q) + 1));
    ch = wgetch(win);
    return (ch == 'y') || (ch == 'Y');
}

static int exec_key_binding(int keycode)
{
#ifdef HAVE_LUA
    struct lua_State* L = global.vm_state;
    lua_getglobal(L, "gmtool_on_keypressed");
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, keycode);
        return lua_pcall(L, 1, 1, 0);
    }
#endif
    return -1;
}

static void show_help(void) 
{
    WINDOW *wn, *pad;
    int line, col, ch=0;
    int height, width;
    const char* const help[] = {
        "",
        "GETTING AROUND",
        "",
        "arrow keys, position keys: move cursor",
        "g: go to coordinate",
        "/: search for ... r: region by name, u: unit by id, f: faction by id, F: faction from list",
        "n: find next element according to last search",
        "SPACE: select / unselect current region",
        "TAB: jump to next selected region (by space or t)",
        "p: jump between planes",
        "a: jump to corresponding astral region / real region",
        "",
        "",
        "DISPLAY",
        "",
        "I: show/hide info about ... f: factions, u: units, s: ships, b: buildings",
        "d: map mode ... t: show terrains, l: show luxurues",
        "",
        "",
        "MANIPULATING DATA",
        "",
        "O: open data file",
        "S: save data file",
        "f, Ctrl-t: terraform region at cursor",
        "CTRL+b: fill block with oceans",
        "B: build island E3 style",
        "s: seed next player from newfactions at current region",
        "A: reset area (set region age to 0) for whole contiguos region",
        "c: clear (reset resources) region under cursor",
        "C: clear rectangle under cursor (2 regions up and to the right)",
        "",
        "h: mark regions ... n: none,   i: island under cursor, t: terrain type, s: with ships,",
        "   u: with units, p: with player units, m: with monsters, f: with units of a faction,",
        "   c: chaos regions, v: new regions with age 0",
        "H: unmark regions (as above)",
        "t: select regions (for batch commands, as above)",
        "T: un-select regions (as above)",
        ";: run batch command for selected regions ... 'r': reset region, 't': terraform', 'f': fix (very special)",
        "",
        "",
        "OTHER",
        "",
        "Ctrl+L: redraw",
        "L: open lua prompt (exit/execute with enter)",
        "Q: quit",
        "" };
    int lines = sizeof help / sizeof *help, cols = 0;
    const char* title = "HELP (exit with q)";
    const int BORDERX = 2, BORDERY = 2;
    bool exit = FALSE;

    for (line = 0; line < lines; ++line) {
       if (cols < (int) strlen(help[line])) cols = (int) strlen(help[line]);
    }
 
    getmaxyx(stdscr, height, width); 

    wn = newwin(height - 2 * BORDERY, width - 2 * BORDERY, 2, 2);
    pad = newpad(lines, cols);
    box(wn, 0, 0);
    mvwprintw(wn, 0, 2, "[ %s ]", title);
    for (line=0;line<lines;++line) {
        wmove(pad, line, 0);
        waddstr(pad, help[line]);
        waddstr(pad, "\n");
    }
    line = 0;
    col = 0;
    wrefresh(wn);
    prefresh(pad, line, 0, BORDERY + 1, BORDERX + 2, height - BORDERY * 2, width - BORDERX * 2);

    while(!exit)
    {
        switch (ch)
        {
        case 'q':
        case 'Q':
        case 27:             /* esc */
            exit = true;
            break;
        case KEY_UP:
            --line;
            break;
        case KEY_DOWN:
            ++line;
            break;
        case KEY_RIGHT:
            col += width / 2 - 2 * BORDERX;
            break;
        case KEY_LEFT:
            col -= width / 2 - 2 * BORDERX;
            break;
        case FAST_UP: 
            line -= height - 2 - 2 * BORDERY;
            break;
        case FAST_DOWN:
            line += height - 2 - 2 * BORDERY;
            break;
        case KEY_END: 
            col = cols - width / 2 + BORDERX * 2;
            break;
        case KEY_HOME:
            col = 0;
            break;
        }
        if (line < 0) line = 0;
        if (line > lines - 1) line = lines - 1;
        if (col > cols - width / 2 + BORDERX * 2) col = cols - width / 2 + BORDERX * 2;
        if (col < 0) col = 0;

        box(wn, 0, 0);
        mvwprintw(wn, 0, 2, "[ %s %d/%d %d/%d]", title, line+1, lines, col+1, cols);
        wnoutrefresh(wn);
        wrefresh(wn);
        prefresh(pad, line, col, BORDERY + 1, BORDERX + 2, height - BORDERY * 2, width - BORDERX * 2);

        if (!exit) ch = getch();
    }
    delwin(pad);
    delwin(wn);
}

static void handlekey(state * st, int c)
{
    window *wnd;
    coordinate *cursor = &st->cursor;
    static char locate[80];
    static int findmode = 0;
    region *r;
    char sbuffer[80];
    const char *loc = locate;
    int n, nx, ny, minpop, maxpop;

    switch (c) {
    case FAST_RIGHT:
        cursor->x += 10;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case FAST_LEFT:
        cursor->x -= 10;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case FAST_UP:
        cursor->y += 10;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case FAST_DOWN:
        cursor->y -= 10;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case KEY_UP:
        cursor->y++;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case KEY_DOWN:
        cursor->y--;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case KEY_RIGHT:
        cursor->x++;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case KEY_LEFT:
        cursor->x--;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        break;
    case 'S':
    case KEY_SAVE:
        savedata(st);
        break;
    case 'O':
    case KEY_OPEN:
        loaddata(st);
        break;
    case '?': /* help */
        show_help();
        st->modified = 1;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        st->wnd_map->update |= 3;
        break;
    case 'B':
        cnormalize(&st->cursor, &nx, &ny);
        minpop = config_get_int("editor.island.min", 8);
        maxpop = config_get_int("editor.island.max", minpop);
        if (maxpop > minpop) {
            n = rng_int() % (maxpop - minpop) + minpop;
        }
        else {
            n = minpop;
        }
        build_island(nx, ny, n, NULL, 0);
        st->modified = 1;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        st->wnd_map->update |= 1;
        break;
    case 0x02:                 /* CTRL+b */
        cnormalize(&st->cursor, &nx, &ny);
        make_block(nx, ny, 6, newterrain(T_OCEAN));
        st->modified = 1;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        st->wnd_map->update |= 1;
        break;
    case 'A': /* clear/reset area */
        if (confirm(st->wnd_status->handle, "Are you sure you want to reset this entire area?")) {
            reset_area(st);
            st->modified = 1;
            st->wnd_map->update |= 1;
        }
        break;
    case 'c': /* clear/reset */
        reset_cursor(st);
        st->modified = 1;
        st->wnd_map->update |= 1;
        break;
    case 'C': /* clear/reset */
        reset_rect(st);
        st->modified = 1;
        st->wnd_map->update |= 1;
        break;
    case 0x09:                 /* tab = next selected */
        if (regions != NULL) {
            map_region *mr = cursor_region(&st->display, cursor);
            if (mr) {
                region *first = mr->r;
                region *cur = (first && first->next) ? first->next : regions;

                while (cur != first) {
                    coordinate coord;
                    region2coord(cur, &coord);
                    cnormalize(&coord, &nx, &ny);
                    if (tagged_region(st->selected, nx, ny)) {
                        st->cursor = coord;
                        st->wnd_info->update |= 1;
                        st->wnd_status->update |= 1;
                        break;
                    }
                    cur = cur->next;
                    if (!cur && first)
                        cur = regions;
                }
            }
        }
        break;

    case 'p':
        if (planes) {
            plane *pl = planes;
            if (cursor->pl) {
                while (pl && pl != cursor->pl) {
                    pl = pl->next;
                }
                if (pl && pl->next) {
                    cursor->pl = pl->next;
                }
                else {
                    cursor->pl = get_homeplane();
                }
            }
            else {
                cursor->pl = planes;
            }
        }
        break;

    case 'a':
        if (regions != NULL) {
            map_region *mr = cursor_region(&st->display, cursor);
            if (mr && mr->r) {
                region *cur = mr->r;
                plane *pl = rplane(cur);
                if (pl == NULL) {
                    cur = r_standard_to_astral(cur);
                }
                else if (is_astral(cur)) {
                    cur = r_astral_to_standard(cur);
                }
                else {
                    cur = NULL;
                }
                if (cur != NULL) {
                    region2coord(cur, &st->cursor);
                }
                else {
                    beep();
                }
            }
        }
        break;
    case 'g':
        askstring(st->wnd_status->handle, "goto-x:", sbuffer, 12);
        if (sbuffer[0]) {
            askstring(st->wnd_status->handle, "goto-y:", sbuffer + 16, 12);
            if (sbuffer[16]) {
                st->cursor.x = atoi(sbuffer);
                st->cursor.y = atoi(sbuffer + 16);
                st->wnd_info->update |= 1;
                st->wnd_status->update |= 1;
            }
        }
        break;
    case 'f':
    case 0x14:                 /* C-t */
        terraform_at(&st->cursor, select_terrain(st, NULL));
        st->modified = 1;
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        st->wnd_map->update |= 3;
        break;
    case 'I':
        statusline(st->wnd_status->handle, "info-");
        doupdate();
        do {
            c = getch();
            switch (c) {
            case 's':
                st->info_flags ^= IFL_SHIPS;
                if (st->info_flags & IFL_SHIPS)
                    statusline(st->wnd_status->handle, "info-ships true");
                else
                    statusline(st->wnd_status->handle, "info-ships false");
                break;
            case 'b':
                st->info_flags ^= IFL_BUILDINGS;
                if (st->info_flags & IFL_BUILDINGS)
                    statusline(st->wnd_status->handle, "info-buildings true");
                else
                    statusline(st->wnd_status->handle, "info-buildings false");
                break;
            case 'f':
                st->info_flags ^= IFL_FACTIONS;
                if (st->info_flags & IFL_FACTIONS)
                    statusline(st->wnd_status->handle, "info-factions true");
                else
                    statusline(st->wnd_status->handle, "info-factions false");
                break;
            case 'u':
                st->info_flags ^= IFL_UNITS;
                if (st->info_flags & IFL_UNITS)
                    statusline(st->wnd_status->handle, "info-units true");
                else
                    statusline(st->wnd_status->handle, "info-units false");
                break;
            case 27:             /* esc */
                break;
            default:
                beep();
                c = 0;
            }
        } while (c == 0);
        break;
    case 'L':
#ifdef HAVE_LUA
        if (global.vm_state) {
            move(0, 0);
            refresh();
            lua_do((struct lua_State *)global.vm_state);
            /* todo: do this from inside the script */
            clear();
            st->wnd_info->update |= 1;
            st->wnd_status->update |= 1;
            st->wnd_map->update |= 3;
        }
#else
        beep();
#endif
        break;
    case 12:                   /* Ctrl-L */
        clear();
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
        st->wnd_map->update |= 1;
        break;
    case 'h':
        select_regions(st, MODE_MARK);
        break;
    case 'H':
        select_regions(st, MODE_UNMARK);
        break;
    case 't':
        select_regions(st, MODE_SELECT);
        break;
    case 'T':
        select_regions(st, MODE_UNSELECT);
        break;
    case ';':
        statusline(st->wnd_status->handle, "tag-");
        doupdate();
        switch (getch()) {
        case 'r':
            reset_selection(st->selected);
            break;
        case 'f':
            fix_selection(st->selected);
            break;
        case 't':
            terraform_selection(st->selected, select_terrain(st, NULL));
            st->modified = 1;
            st->wnd_info->update |= 1;
            st->wnd_status->update |= 1;
            st->wnd_map->update |= 1;
            break;
        case 'm':
            break;
        default:
            statusline(st->wnd_status->handle, "unknown command.");
            beep();
        }
        break;
    case ' ':
        cnormalize(cursor, &nx, &ny);
        if (tagged_region(st->selected, nx, ny))
            untag_region(st->selected, nx, ny);
        else
            tag_region(st->selected, nx, ny);
        break;
    case 's': /* seed */
        if (new_players) {
            newfaction * next = new_players->next;
            if (seed_player(st, new_players)) {
                free(new_players->email);
                free(new_players->password);
                free(new_players);
                new_players = next;
                st->wnd_info->update |= 1;
                /*st->wnd_map->update |= 3;*/
            }
        }
        break;
    case '/':
        statusline(st->wnd_status->handle, "find-");
        doupdate();
        findmode = getch();
        if (findmode == 'r') {
            askstring(st->wnd_status->handle, "find-region:", locate,
                sizeof(locate));
        }
        else if (findmode == 'u') {
            askstring(st->wnd_status->handle, "find-unit:", locate, sizeof(locate));
        }
        else if (findmode == 'f') {
            askstring(st->wnd_status->handle, "find-faction:", locate,
                sizeof(locate));
        }
        else if (findmode == 'F') {
            faction *f = select_faction(st);
            if (f != NULL) {
                loc = itoa36_r(f->no, locate, sizeof(locate));
                findmode = 'f';
            }
            else {
                break;
            }
        }
        else {
            statusline(st->wnd_status->handle, "unknown command.");
            beep();
            break;
        }
        /* achtung: fall-through ist absicht: */
        if (!strlen(loc)) {
            break;
        }
        /* intentional fallthrough */
    case 'n':
        if (findmode == 'u') {
            unit *u = findunit(atoi36(locate));
            r = u ? u->region : NULL;
        }
        else if (findmode && regions != NULL) {
            struct faction *f = NULL;
            map_region *mr = cursor_region(&st->display, cursor);
            region *first = (mr && mr->r && mr->r->next) ? mr->r->next : regions;

            if (findmode == 'f') {
                snprintf(sbuffer, sizeof(sbuffer), "find-faction: %.40s", loc);
                statusline(st->wnd_status->handle, sbuffer);
                f = findfaction(atoi36(loc));
                if (f == NULL) {
                    statusline(st->wnd_status->handle, "faction not found.");
                    beep();
                    break;
                }
            }
            for (r = first;;) {
                if (findmode == 'r' && r->land && r->land->name
                    && strstr((const char *)r->land->name, loc)) {
                    break;
                }
                else if (findmode == 'f') {
                    unit *u;
                    for (u = r->units; u; u = u->next) {
                        if (u->faction == f) {
                            break;
                        }
                    }
                    if (u)
                        break;
                }
                r = r->next;
                if (r == NULL)
                    r = regions;
                if (r == first) {
                    r = NULL;
                    statusline(st->wnd_status->handle, "not found.");
                    beep();
                    break;
                }
            }
        }
        else {
            r = NULL;
        }
        if (r != NULL) {
            region2coord(r, &st->cursor);
            st->wnd_info->update |= 1;
            st->wnd_status->update |= 1;
        }
        break;
    case 'd':
        statusline(st->wnd_status->handle, "draw-");
        doupdate();
        findmode = getch();
        if (findmode == 't') {
            statusline(st->wnd_status->handle, "draw-terrain");
            st->wnd_map->data = (void*)draw_terrain;
            st->wnd_map->update |= 1;
        }
        else if (findmode == 'l') {
            statusline(st->wnd_status->handle, "draw-luxury");
            st->wnd_map->data = (void*)draw_luxury;
            st->wnd_map->update |= 1;
        }
        break;
    case 'Q':
        g_quit = 1;
        break;
    default:
        if (exec_key_binding(c) < 0) {
            for (wnd = wnd_first; wnd != NULL; wnd = wnd->next) {
                if (wnd->handlekey) {
                    if (wnd->handlekey(wnd, st, c))
                        break;
                }
            }
        }
        break;
    }
}

static void init_view(view * display, WINDOW * win)
{
    display->topleft.x = 1;
    display->topleft.y = 1;
    display->topleft.pl = get_homeplane();
    display->pl = get_homeplane();
    display->size.width = getmaxx(win) / TWIDTH;
    display->size.height = getmaxy(win) / THEIGHT;
    display->regions =
        calloc(display->size.height * (size_t)display->size.width, sizeof(map_region));
}

static void update_view(view * vi)
{
    int i, j;
    for (i = 0; i != vi->size.width; ++i) {
        for (j = 0; j != vi->size.height; ++j) {
            map_region *mr = mr_get(vi, i, j);
            mr->coord.x = vi->topleft.x + i - j / 2;
            mr->coord.y = vi->topleft.y + j;
            mr->coord.pl = vi->pl;
            pnormalize(&mr->coord.x, &mr->coord.y, mr->coord.pl);
            mr->r = findregion(mr->coord.x, mr->coord.y);
        }
    }
}

static void close_view(view *display) {
    free(display->regions);
}

state *state_open(void)
{
    state *st = (state *)calloc(1, sizeof(state));
    if (st) {
        st->display.pl = get_homeplane();
        st->cursor.pl = get_homeplane();
        st->cursor.x = 0;
        st->cursor.y = 0;
        st->selected = calloc(1, sizeof(struct selection));
        st->modified = 0;
        st->info_flags = 0xFFFFFFFF;
        st->prev = current_state;
        current_state = st;
    }
    return st;
}

void state_close(state * st)
{
    assert(st == current_state);
    current_state = st->prev;
    free(st->selected);
    free(st);
}

void run_mapper(void)
{
    WINDOW *hwinstatus;
    WINDOW *hwininfo;
    WINDOW *hwinmap;
    int width, height, x, y;
    int split = 30;
    state *st;
    point tl;

    if (!new_players) {
        char sbuffer[512];
        path_join(basepath(), "newfactions", sbuffer, sizeof(sbuffer));
        new_players = read_newfactions(sbuffer);
    }

    init_curses();
    curs_set(1);
#ifdef HAVE_LUA
    set_readline(curses_readline);
#endif
    assert(stdscr);
    getbegyx(stdscr, x, y);
    width = getmaxx(stdscr);
    height = getmaxy(stdscr);

    hwinmap = subwin(stdscr, getmaxy(stdscr) - 1, getmaxx(stdscr) - split, y, x);
    hwininfo =
        subwin(stdscr, getmaxy(stdscr) - 1, split, y, x + getmaxx(stdscr) - split);
    hwinstatus = subwin(stdscr, 1, width, height - 1, x);

    st = state_open();
    st->wnd_map = win_create(hwinmap);
    st->wnd_map->paint = &paint_map;
    st->wnd_map->update = 1;
    st->wnd_info = win_create(hwininfo);
    st->wnd_info->paint = &paint_info_default;
    st->wnd_info->handlekey = &handle_info_region;
    st->wnd_info->update = 1;
    st->wnd_status = win_create(hwinstatus);
    st->wnd_status->paint = &paint_status;
    st->wnd_status->update = 1;

    init_view(&st->display, hwinmap);
    coor2point(&st->display.topleft, &tl);

    hstatus = st->wnd_status->handle;     /* the lua console needs this */

    while (!g_quit) {
        int c;
        point p;
        window *wnd;
        view *vi = &st->display;

        getbegyx(hwinmap, x, y);
        coor2point(&st->cursor, &p);

        if (st->cursor.pl != vi->pl) {
            vi->pl = st->cursor.pl;
            st->wnd_map->update |= 1;
        }
        if (p.y < tl.y) {
            vi->topleft.y = st->cursor.y - vi->size.height / 2;
            st->wnd_map->update |= 1;
        }
        else if (p.y >= tl.y + vi->size.height * THEIGHT) {
            vi->topleft.y = st->cursor.y - vi->size.height / 2;
            st->wnd_map->update |= 1;
        }
        if (p.x <= tl.x) {
            vi->topleft.x =
                st->cursor.x + (st->cursor.y - vi->topleft.y) / 2 - vi->size.width / 2;
            st->wnd_map->update |= 1;
        }
        else if (p.x >= tl.x + vi->size.width * TWIDTH - 1) {
            vi->topleft.x =
                st->cursor.x + (st->cursor.y - vi->topleft.y) / 2 - vi->size.width / 2;
            st->wnd_map->update |= 1;
        }

        if (st->wnd_map->update) {
            update_view(vi);
            coor2point(&vi->topleft, &tl);
        }
        for (wnd = wnd_last; wnd != NULL; wnd = wnd->prev) {
            if (wnd->update && wnd->paint) {
                if (wnd->update & 1) {
                    wnd->paint(wnd, st);
                    wnoutrefresh(wnd->handle);
                }
                if (wnd->update & 2) {
                    touchwin(wnd->handle);
                }
                wnd->update = 0;
            }
        }
        draw_cursor(st->wnd_map, st->selected, vi, &st->cursor, 1);
        doupdate();
        c = getch();
        draw_cursor(st->wnd_map, st->selected, vi, &st->cursor, 0);
        handlekey(st, c);
    }
    g_quit = 0;
#ifdef HAVE_LUA
    set_readline(NULL);
#endif
    curs_set(1);
    endwin();
    /* FIXME: reset logging
        log_flags = old_flags;
    */
    untag_all_regions(st->selected);
    close_view(&st->display);
    state_close(st);

}

#ifdef HAVE_LUA
int
curses_readline(struct lua_State *L, char *buffer, size_t size,
    const char *prompt)
{
    UNUSED_ARG(L);
    askstring(hstatus, prompt, buffer, size);
    return buffer[0] != 0;
}
#endif

void seed_players(newfaction **players, bool new_island)
{
    if (players) {
        while (*players) {
            int n = listlen(*players);
            int k = (n + ISLANDSIZE - 1) / ISLANDSIZE;
            k = n / k;
            n = autoseed(players, k, new_island ? 0 : TURNS_PER_ISLAND);
            if (n == 0) {
                break;
            }
        }
    }
}

void make_block(int x, int y, int radius, const struct terrain_type *terrain)
{
    int cx, cy;
    region *r;
    plane *pl = findplane(x, y);

    if (terrain == NULL)
        return;

    for (cx = x - radius; cx != x + radius; ++cx) {
        for (cy = y - radius; cy != y + radius; ++cy) {
            int nx = cx, ny = cy;
            pnormalize(&nx, &ny, pl);
            if (koor_distance(nx, ny, x, y) < radius) {
                if (!findregion(nx, ny)) {
                    r = new_region(nx, ny, pl, 0);
                    terraform_region(r, terrain);
                }
            }
        }
    }
}
