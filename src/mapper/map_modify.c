/* vi: set ts=2:
 *
 *	$Id: map_modify.c,v 1.3 2001/02/03 13:45:34 enno Exp $
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

#define BOOL_DEFINED
/* wenn config.h nicht vor curses included wird, kompiliert es unter windows nicht */
/* wenn curses.h nicht vor mapper included wird, kennt es die structs nicht. TODO: curses-teil separieren (map_tools.h) */
#include <config.h>
#include <curses.h>
#include <eressea.h>
#include "mapper.h"

/* kernel includes */
#include <build.h>
#include <building.h>
#include <faction.h>
#include <item.h>
#include <movement.h>
#include <plane.h>
#include <region.h>
#include <ship.h>
#include <terrain.h>
#include <unit.h>

/* util includes */
#include <base36.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

typedef struct menulist {
	struct menulist *next, *prev;
	int *val;
	char *text;
} menulist;

enum {
	C_COOL,
	C_TEMPERATE,
	C_DRY,
	C_TROPIC,
	MAXCLIMATES
};

static int
climate(int y)
{
	if (y < -BLOCKSIZE)
		return C_COOL;
	if (y > 2 * BLOCKSIZE)
		return C_TROPIC;
	if (y > BLOCKSIZE)
		return C_DRY;
	return C_TEMPERATE;
}

#define MAXSEEDSIZE 17
static char maxseeds[MAXCLIMATES][MAXTERRAINS] =
{
	{0, 1, 3, 3, 0, 3, 3, 4,},	/* Summe muß MAXSEEDSIZE sein */
	{0, 5, 4, 2, 0, 2, 3, 1,},
	{0, 6, 1, 0, 6, 2, 2, 0,},
	{0, 4, 5, 4, 0, 2, 2, 0,},
};

static terrain_t
terrain_create(int climate)
{
	int i = rand() % MAXSEEDSIZE;
	terrain_t terrain = T_OCEAN;

	while (i > maxseeds[climate][terrain])
		i -= maxseeds[climate][terrain++];
	return terrain;
}

static int
blockcoord(int x)
{
	return ((x<0)?((x-BLOCKSIZE+1) / BLOCKSIZE):(x / BLOCKSIZE)) * BLOCKSIZE;
}

static char newblock[BLOCKSIZE][BLOCKSIZE];
static int g_maxluxuries;

static void
block_create(int x1, int y1, int size, char chaotisch, int special, char terrain)
{
	int local_climate;
	int x, y, k;
	vset fringe;

	vset_init(&fringe);

	x1 = blockcoord(x1);
	y1 = blockcoord(y1);
	local_climate = climate(y1);

	memset(newblock, T_OCEAN, sizeof newblock);
	x = BLOCKSIZE / 2;
	y = BLOCKSIZE / 2;

	vset_add(&fringe, (void *) (((short) (x) << 16) +
								((short) (y) & 0xFFFF)));
	for (x=0;x!=BLOCKSIZE;++x) {
		for (y=0;y!=BLOCKSIZE;++y) {
			/* add the borders of the block taht have a
			 * non-ocean region in them to the fringe
			 * */
			int i;
			direction_t d;
			int nb[4][3];

			memset(nb, 0, sizeof(nb));

			for (d=0;d!=MAXDIRECTIONS;++d) {
				region * r = findregion(x1 + delta_x[d] + x, y1 + delta_y[d]);
				if (r && rterrain(r)!=T_OCEAN) {
					nb[0][0] = x;
					nb[0][1] = 0;
					nb[0][2] = 1;
				}
				r = findregion(x1 + delta_x[d] + x, y1 + BLOCKSIZE - 1 + delta_y[d]);
				if (r && rterrain(r)!=T_OCEAN) {
					nb[1][0] = x;
					nb[1][1] = BLOCKSIZE - 1;
					nb[1][2] = 1;
				}
				r = findregion(x1 + delta_x[d] + BLOCKSIZE - 1, y1 + y + delta_y[d]);
				if (r && rterrain(r)!=T_OCEAN) {
					nb[2][0] = BLOCKSIZE - 1;
					nb[2][1] = y;
					nb[2][2] = 1;
				}
				r = findregion(x1 + delta_x[d], y1 + y + delta_y[d]);
				if (r && rterrain(r)!=T_OCEAN) {
					nb[3][0] = 0;
					nb[3][1] = y;
					nb[3][2] = 1;
				}
			}

			for (i=0;i!=3;++i) if (nb[i][2]) {
				vset_add(&fringe, (void *) (((short) nb[i][0] << 16) +
											((short) nb[i][1] & 0xFFFF)));
			}
		}
	}
	for (k = 0; k != size; ++k) {
		int c = (int) fringe.data[rand() % fringe.size];
		direction_t d;

		x = (c & 0xFFFF0000) >> 16;
		y = (c & 0xFFFF);
		assert(newblock[x][y] == T_OCEAN);
		newblock[x][y] = terrain_create(local_climate);
		vset_erase(&fringe, (void *) c);
		for (d = 0; d != MAXDIRECTIONS; ++d) {
			int dx = x+delta_x[d];
			int dy = y+delta_y[d];
			if (dx >= 0 && dx < BLOCKSIZE && dy >= 0 && dy < BLOCKSIZE && newblock[dx][dy] == T_OCEAN)
				vset_add(&fringe, (void *) (((short) (dx) << 16) +
											((short) (dy) & 0xFFFF)));
		}
	}
	{
		/* newblock wird innerhalb der Rahmen in die Karte kopiert, die
		 * Landstriche werden benannt und bevoelkert, und die produkte
		 * p1 und p2 des Kontinentes werden gesetzt. */
		region *r;
		int i, i1, i2;
		const luxury_type *ltype, *p1 = NULL, *p2=NULL;
		if (g_maxluxuries==0) {
			for (ltype = luxurytypes;ltype;ltype=ltype->next) ++g_maxluxuries;
		}
		i1 = (item_t)(rand() % g_maxluxuries);
		do {
			i2 = (item_t)(rand() % g_maxluxuries);
		}
		while (i2 == i1);
		ltype = luxurytypes;
		for (i=0;!p1 || !p2;++i) {
			if (i==i1) p1=ltype;
			else if (i==i2) p2=ltype;
			ltype=ltype->next;
		}
		for (x = 0; x != BLOCKSIZE; x++) {
			for (y = 0; y != BLOCKSIZE; y++) {
				const luxury_type * sale = (rand()%2)?p1:p2;
				r = findregion(x1 + x, y1 + y);
				if (r) continue;
				r = new_region(x1 + x, y1 + y);
				if (chaotisch) fset(r, RF_CHAOTIC);
				if (special == 1) {
					terraform(r, terrain);
				} else if (special == 2) {
					if (newblock[x][y] != T_OCEAN)
						terraform(r, terrain);
					else
						terraform(r, T_OCEAN);
				} else {
					terraform(r, newblock[x][y]);
				}
				if (r->land) setluxuries(r, sale);
			}
		}
	}
	vset_destroy(&fringe);
}


static void
blockcreate(int x1, int y1, int size, char chaos)
{
	char chaotic = 0;
	int special = 0;
	char terrain = T_OCEAN;
	int local_climate = climate(y1);

	if (special == 1)
		terrain = T_OCEAN;
	else if (special == 2)
		terrain = terrain_create(local_climate);

	/* Chaotisch ? */

	if ((x1 < -36 || x1 > 36 || y1 < -36) && chaos == 1) {
		if (rand() % 100 < 50) {
			chaotic = 1;
		}
		switch (rand() % 6) {
		case 1:
		case 2:
			special = 1;		/* Nur Wasser ... */
			break;
		case 3:
			special = 2;		/* Ein-Terrain-Insel */
			break;
		case 4:				/* Miniinsel */
			size = 3 + rand() % 5;
			break;
		}
	}
	block_create(x1, y1, size, chaotic, special, terrain);
}

void
makeblock(int x1, int y1, char chaos)
{
	blockcreate(x1, y1, ISLANDSIZE, chaos);
}

static void
addmenulist(menulist ** SP, const char *s, int *val)
{
	menulist *S, *X;
	S = calloc(1, sizeof(menulist));
	S->text = strdup(s);
	S->val = val;
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

/* Wilder Hack: Zur Übergabe an addmenulist() brauchen wir Adressen,
 * die die r*-Funktionen natürlich nicht mehr liefern. Also diese
 * Variablen als Zwischenpuffer. */

static int peasants, money, trees, horses, iron, laen, chaotisch;

static void
get_region(region *r) {
	peasants  = rpeasants(r);
	money     = rmoney(r);
	trees     = rtrees(r);
	horses    = rhorses(r);
	iron      = riron(r);
	laen      = rlaen(r);
	chaotisch = fval(r, RF_CHAOTIC);
}

static void
put_region(region *r) {
	rsetpeasants(r, peasants);
	rsetmoney(r,money);
	rsettrees(r,trees);
	rsethorses(r, horses);
	rsetiron(r, iron);
	rsetlaen(r, laen);
	if (chaotisch) fset(r, RF_CHAOTIC); else freset(r, RF_CHAOTIC);
}

static void
create_region_menu(menulist ** menu, region * r)
{
	char str[80];
	unit *u;
	building *b;
	ship *sh;
	faction *f;

	get_region(r);

	addmenulist(menu, "Bauern", &peasants);
	addmenulist(menu, "Silber", &money);
	if (fval(r, RF_MALLORN))
		addmenulist(menu, "Mallornbäume", &trees);
	else
		addmenulist(menu, "Bäume", &trees);
	addmenulist(menu, "Pferde", &horses);
	if (r->terrain == T_MOUNTAIN || r->terrain == T_GLACIER) {
		addmenulist(menu, "Eisen", &iron);
		addmenulist(menu, "Laen", &laen);
	}
	addmenulist(menu, "Chaos-Faktor", &chaotisch);

	if (r->planep) {
		strcpy(buf,"Plane: ");
		strncpy(str, r->planep->name, 30);
		str[30] = 0;
		sncat(buf, str, BUFSIZE);
		sncat(buf, " (", BUFSIZE);
		sncat(buf, r->planep->name, BUFSIZE);
		sncat(buf, ")", BUFSIZE);
		addmenulist(menu, buf, 0);
	}
	strcpy(buf, "Burgen:");
	if (!r->buildings) {
		sncat(buf, " keine", BUFSIZE);
		addmenulist(menu, buf, 0);
	} else {
		addmenulist(menu, buf, 0);
		for (b = r->buildings; b; b = b->next) {
			sprintf(buf, " %s: ", Buildingid(b));
			for (u = r->units; u; u = u->next)
				if (u->building == b && fval(u, FL_OWNER)) {
					strncpy(str, u->name, 28);
					str[28] = 0;
					sncat(buf, str, BUFSIZE);
					sprintf(str, " (%s), Partei %s", unitid(u), factionid(u->faction));
					sncat(buf, str, BUFSIZE);
					break;
				}
			if (!u)
				sncat(buf, "steht leer", BUFSIZE);
			addmenulist(menu, buf, 0);
		}
	}

	strcpy(buf, "Schiffe:");
	if (!r->ships) {
		sncat(buf, " keine", BUFSIZE);
		addmenulist(menu, buf, 0);
	} else {
		addmenulist(menu, buf, 0);
		for (sh = r->ships; sh; sh = sh->next) {
			sprintf(buf, " %s: ", shipname(sh));
			if (sh->size!=sh->type->construction->maxsize)
				sncat(buf, "(im Bau) ", BUFSIZE);
			for (u = r->units; u; u = u->next)
				if (u->ship == sh && fval(u, FL_OWNER)) {
					strncpy(str, u->name, 28);
					str[28] = 0;
					sncat(buf, str, BUFSIZE);
					sprintf(str, " (%s), Partei %s", unitid(u), factionid(u->faction));
					sncat(buf, str, BUFSIZE);
					break;
				}
			if (!u)
				sncat(buf, "ohne Besitzer", BUFSIZE);
			addmenulist(menu, buf, 0);
		}
	}

	strcpy(buf, "Parteien:");

	if (!r->units) {
		sncat(buf, " keine", BUFSIZE);
		addmenulist(menu, buf, 0);
	} else {
		for (f = factions; f; f = f->next)
			f->num_people = f->nunits = 0;
		for (u = r->units; u; u = u->next) {
			u->faction->nunits++;
			u->faction->num_people += u->number;
		}
		addmenulist(menu, buf, 0);

		for (f = factions; f; f = f->next) {
			if (f->nunits) {
				sprintf(buf, " %s: ", factionname(f));
				sprintf(str, "Einheiten: %d; Leute: %d", f->nunits, f->num_people);
				sncat(buf, str, BUFSIZE);
				addmenulist(menu, buf, 0);
			}
		}
	}
}

char modified = 0;

WINDOW *mywin;
int pline;

#define Printw(x,l) \
	{ \
		wmove(mywin,l,4); \
		Addstr(x->text); \
		if (x->val) wprintw(mywin, (NCURSES_CONST char*)": %d",(char*)*(x->val)); \
	} \

static void
modify_value(menulist * v)
{
	int vl;
	vl = map_input(mywin, 40, pline, "Neuer Wert", -1, 999999, *(v->val));
	Movexy(4, pline);
	wclrtoeol(mywin);
	Printw(v, pline);
	if (vl != *(v->val)) {
		modified = 1;
		*(v->val) = vl;
	}
}

void
NeuesSchiff(region * r)
{
	ship_type *stype[40]; /* Maximal 40 Schiffe */
	ship_typelist *st;
	ship *s;
	WINDOW *win;
	int i, q, y, maxtype;

	for(st = shiptypes, maxtype=0; st; st=st->next, maxtype++) {
		stype[maxtype] = (ship_type *)st->type;
	}

	win = openwin(SX - 10, 6, "< Neues Schiff erschaffen >");

	q = 0;
	y = 2;
	wmove(win, y, 4);
	for (i = 0; i < maxtype; i++) {
		sprintf(buf, "%d=%s; ", i, stype[i]->name[0]);
		q += strlen(buf);
		if (q > SX - 20) {
			q = strlen(buf);
			y++;
			wmove(win, y, 4);
		}
		waddnstr(win, buf, -1);
	}
	wrefresh(win);
	q = map_input(win, 2, 1, "Schiffstyp", -1, maxtype, 0);

	if (q < 0) {
		delwin(win);
		return;
	}
	for (; y > 0; y--) {
		wmove(win, y, 2);
		wclrtoeol(win);
		wrefresh(win);
		wmove(win, y, win->_maxx);
		waddch(win, '|');
		wrefresh(win);
	}
	wmove(win, 1, 2);
	wAddstr(stype[q]->name[0]);
	wrefresh(win);

	s = new_ship(stype[q]);
	/* Ist das richtig so, Henning? */
	addlist(&r->ships, s);

	strcpy(buf, my_input(win, 2, 2, "Name: "));
	if (strlen(buf) > 0)
		set_string(&s->name, buf);
	if (clipunit) {
		wmove(win, 3, 2);
		wAddstr("Einheit im Clip: ");
		wAddstr(Unitid(clipunit));
		wmove(win, 4, 2);
		if (yes_no(win, "Clipunit als Besitzer?", 'n')) {
			leave(clipregion, clipunit);
			if (r != clipregion)
				translist(&clipregion->units, &r->units, clipunit);
			clipunit->ship = s;
			fset(clipunit, FL_OWNER);
		}
	}
	delwin(win);
	modified = 1;
}

void
NeueBurg(region * r)
{
	building *b;
	WINDOW *win;
	int i, q, y;
	win = openwin(SX - 10, 10, "< Neues Gebäude erschaffen >");

	q = 0;
	y = 2;
	wmove(win, y, 4);
	for (i = 0; i < MAXBUILDINGTYPES; i++) {
		sprintf(buf, "%d=%s; ", i, oldbuildings[i]->_name);
		q += strlen(buf);
		if (q > SX - 20) {
			q = strlen(buf);
			y++;
			wmove(win, y, 4);
		}
		waddnstr(win, buf, -1);
	}
	wrefresh(win);
	q = map_input(win, 2, 1, "Gebäudetyp", -1, MAXBUILDINGTYPES, 0);

	if (q < 0) {
		delwin(win);
		return;
	}
	for (; y > 0; y--) {
		wmove(win, y, 4);
		wclrtoeol(win);
		wrefresh(win);
		wmove(win, y, win->_maxx);
		waddch(win, '|');
	}
	wmove(win, 1, 2);
	wAddstr((char*)oldbuildings[i]->_name);
	wrefresh(win);

	b = new_building(oldbuildings[q], r, NULL);

	b->size = map_input(win, 2, 2, "Größe", 1, 999, 1);

	strcpy(buf, my_input(win, 2, 3, "Name: "));
	if (strlen(buf) > 0)
		set_string(&b->name, buf);
	if (clipunit) {
		wmove(win, 4, 2);
		wAddstr("Einheit im Clip: ");
		wAddstr(Unitid(clipunit));
		wmove(win, 5, 2);
		if (yes_no(win, "Clipunit als Besitzer?", 'n')) {
			leave(clipregion, clipunit);
			if (r != clipregion)
				translist(&clipregion->units, &r->units, clipunit);
			clipunit->building = b;
			fset(clipunit, FL_OWNER);
		}
	}
	delwin(win);
	modified = 1;
}

int
modify_region(region * r)
{
	menulist *eh = NULL, *unten, *oben, *hlp = NULL, *such = NULL, *mpoint;
	int line, ch, bottom, bot;
	size_t lt;
	char *s = NULL, *txt, *suchtext = NULL;

	create_region_menu(&eh, r);

	clear();
	strncpy(buf, rname(r, NULL), 65);
	buf[65] = 0;
	movexy(0, 0);
	printw((char*)"%s (%d,%d):", buf, r->x, r->y);
	for (line = 0; line <= SX; line++)
		buf[line] = '-';
	movexy(0, SY - 1);
	addstr(buf);
	movexy(0, 1);
	addstr(buf);
	movexy(0, SY);
	addstr("<Ret>: Neuer Wert; T: neuer Text; u: Units; B: neue Burg; S: neues Schiff");
	refresh();

	mywin = newwin(SY - 3, SX, 2, 0);
	wclear(mywin);
	bot = mywin->_maxy + 1;
	for (line = 0, unten = eh; line < bot && unten->next; line++, unten = unten->next) {
		Printw(unten, line);
		wrefresh(mywin);
	}
	Printw(unten, line);
	wrefresh(mywin);

	bottom = line - 1;
	mpoint = oben = eh;	/* unten=unten->prev; */
	scrollok(mywin, TRUE);
	pline = 0;

	for (;;) {
		Movexy(1, pline);
		if (mpoint->val) {
			wattron(mywin, A_BOLD);
			Addstr("=>");
			wattroff(mywin, A_BOLD);
			move(SY, 0);
			clrtoeol();
			addstr("<Ret>: Neuer Wert; T: neuer Text; u: Units; B: neue Burg; S: neues Schiff");
			refresh();
		} else {
			Addstr("->");
			move(SY, 0);
			clrtoeol();
			addstr("u: Units; T: neuer Text; B: neue Burg; S: neues Schiff");
			refresh();
		}
		wrefresh(mywin);
		ch = getch();
		Movexy(1, pline);
		Addstr("  ");
		switch (ch) {
		case KEY_DOWN:
			if (mpoint->next) {
				mpoint = mpoint->next;
				if (pline > bottom) {
					pline--;
					if (unten->next) {
						wscrl(mywin, 1);
						Printw(unten, bottom);
						unten = unten->next;
						oben = oben->next;
					}
				}
				pline++;
			} else
				beep();
			break;
		case KEY_UP:
			if (mpoint->prev) {
				mpoint = mpoint->prev;
				pline--;
				if (pline < 0) {
					pline++;
					if (oben->prev) {
						unten = unten->prev;
						oben = oben->prev;
						wscrl(mywin, -1);
						Printw(oben, 0);
					}
				}
			} else
				beep();
			break;
		case KEY_RIGHT:
		case KEY_NPAGE:
			for (line = 0; line < 20 && unten->next; line++) {
				oben = oben->next;
				unten = unten->next;
				if (mpoint->next)
					mpoint = mpoint->next;
				else
					pline--;
			}
			wclear(mywin);
			for (line = 0, hlp = oben; line < bot && hlp->next; line++, hlp = hlp->next) {
				Printw(hlp, line);
				wrefresh(mywin);
			}
			Printw(hlp, line);
			wrefresh(mywin);
			bottom = line - 1;
			if (pline < 0)
				pline = 0;
			break;
		case KEY_PPAGE:
		case KEY_LEFT:
			for (line = 0; line < 20 && oben->prev; line++) {
				oben = oben->prev;
				unten = unten->prev;
				if (mpoint->prev)
					mpoint = mpoint->prev;
				else
					pline++;
			}
			wclear(mywin);
			for (line = 0, hlp = oben; line < bot && hlp->next; line++, hlp = hlp->next) {
				Printw(hlp, line);
				wrefresh(mywin);
			}
			Printw(hlp, line);
			wrefresh(mywin);
			bottom = line - 1;
			if (pline > bottom)
				pline = bottom;
			break;
		case 12:	/* ^L -> redraw */
			wclear(mywin);
			for (line = 0, hlp = oben; line < bot && hlp->next; line++, hlp = hlp->next) {
				Printw(hlp, line);
				wrefresh(mywin);
			}
			Printw(hlp, line);
			wrefresh(mywin);
			bottom = line - 1;
			break;
		case '/':
			suchtext = my_input(0, 0, 0, (char*)"Suchtext: ");
			such = eh;
		case 'n':
			line = 0;
			lt = strlen(suchtext);
			while (such->next && !line) {
				s = such->text;
				while (strlen(s) >= lt && !line) {
					if (strncasecmp(s, suchtext, lt) == 0)
						line = 1;
					s++;
				}
				such = such->next;
			}
			if (line) {
				wclear(mywin);
				Movexy(4, 0);
				txt = such->prev->text;
				s--;
				ch = *s;
				*s = 0;
				Addstr(txt);
				wattron(mywin, A_REVERSE);
				*s = (char)ch;
				txt = s;
				ch = txt[lt];
				txt[lt] = 0;
				Addstr(txt);
				wattroff(mywin, A_REVERSE);
				txt[lt] = (char)ch;
				txt += lt;
				Addstr(txt);
				if (such->prev->val) {
					sprintf(buf, ": %d", *(such->prev->val));
					Addstr(buf);
				}
				for (line = 0, hlp = oben; line < bot && hlp->next; line++, hlp = hlp->next) {
					Printw(hlp, line);
					wrefresh(mywin);
				}
				Printw(hlp, line);
				wrefresh(mywin);
				bottom = line - 1;
				oben = such->prev ? such->prev : such;
				unten = hlp->prev;
				mpoint = oben;
				pline = 0;
			} else {
				movexy(0, SY);
				beep();
				clrtoeol();
				printw((char*)"'%s' nicht gefunden.", suchtext);
				refresh();
			}
			break;
		case KEY_HELP:
		case '?':
		case 'h':
			movexy(0, SY);
			clrtoeol();
			addstr("<Ret>: Neuer Wert; u: Einheitenliste; B: neue Burg; S: neues Schiff");
			refresh();
			break;
		case '>':
			wclear(mywin);
			do {
				unten = unten->next;
				oben = oben->next;
			} while (unten->next);
			for (line = 0, hlp = oben; line < bot && hlp; line++, hlp = hlp->next) {
				Printw(hlp, line);
				wrefresh(mywin);
			}
			Printw(hlp, line);
			wrefresh(mywin);
			bottom = line - 1;
			mpoint = unten;
			pline = bottom;
			break;
		case '<':
			wclear(mywin);
			for (line = 0, unten = eh; line < bot && unten->next; line++, unten = unten->next) {
				Printw(unten, line);
				wrefresh(mywin);
			}
			Printw(hlp, line);
			wrefresh(mywin);
			bottom = line - 1;
			mpoint = such = oben = eh;
			unten = unten->prev;
			pline = 0;
			break;
		case 10:
		case 13:
			if (mpoint->val) {
				modify_value(mpoint);
				for (line = 0, unten = eh; line < bot && unten->next; line++, unten = unten->next) {
					Printw(unten, line);
					wrefresh(mywin);
				}
				Printw(unten, line);
				wrefresh(mywin);
			} else
				beep();
			break;
		case 'T':

		case 'B':
			if (r->terrain != T_OCEAN) {
				NeueBurg(r);
				return 1;
			} else
				beep();
		case 'S':
			NeuesSchiff(r);
			return 1;
		case 'u':
			while (showunits(r));
			wclear(mywin);
			for (line = 0, hlp = oben; line < bot && hlp->next; line++, hlp = hlp->next) {
				Printw(hlp, line);
				wrefresh(mywin);
			}
			Printw(hlp, line);
			wrefresh(mywin);
			bottom = line - 1;
			break;
		case 27:	/* Esc */
		case 'q':
			put_region(r);
			delwin(mywin);
			freelist(eh);
			return 0;
		}
	}
}

void
make_new_region(int x, int y)
{
	WINDOW *win;
	int q, z, i;
	region *r;

	win = openwin(SX - 10, 10, "< Region erzeugen >");

	x = map_input(win, 2, 1, "X-Koordinate", -999, 999, x);
	y = map_input(win, 2, 2, "Y-Koordinate", -999, 999, y);
	wmove(win, 3, 2);
	if ((r=findregion(x, y))!=NULL) {
		if (!yes_no(win, "Dort ist schon etwas! Überschreiben?", 'n'))
			return;
		else {
			wmove(win, 3, 2);
			wclrtoeol(win);
			wmove(win, 3, win->_maxx);
			waddch(win, '|');
		}
	} else
		r=new_region(x,y);
	z = 4;
	q = 0;
	wmove(win, z, 4);
	for (i = 0; i < MAXTERRAINS; i++) {
		sprintf(buf, "%d=%s; ", i, terrain[i].name);
		q += strlen(buf);
		if (q > SX - 20) {
			q = strlen(buf);
			z++;
			wmove(win, z, 4);
		}
		wAddstr(buf);
	}
	wrefresh(win);
	terraform(r, (terrain_t) map_input(win, 2, 3, "Terraintyp", 0, MAXTERRAINS-1, 0));
	for (; z > 3; z--) {
		wmove(win, z, 2);
		wclrtoeol(win);
		wrefresh(win);
		wmove(win, z, win->_maxx);
		waddch(win, '|');
		wrefresh(win);
	}
	wmove(win, 4, 3);
	if(yes_no(win, "Chaotische Region?", 'n')) {
		fset(r, RF_CHAOTIC);
	} else {
		freset(r, RF_CHAOTIC);
	}
	modified=1;
}

void
make_new_block(int x, int y)
{
	WINDOW *win;
	int q, z, i, special = 0;
	char chaos, t = 0;
	win = openwin(SX - 10, 10, "< Block erzeugen >");

	x = map_input(win, 2, 1, "X-Koordinate innerhalb des Blocks", -999, 999, x);
	y = map_input(win, 2, 2, "Y-Koordinate innerhalb des Blocks", -999, 999, y);
	wmove(win, 3, 2);
	if (findregion(x, y)) {
		if (!yes_no(win, "Dort ist schon etwas! Überschreiben?", 'n'))
			return;
		else {
			wmove(win, 3, 2);
			wclrtoeol(win);
			wmove(win, 3, win->_maxx);
			waddch(win, '|');
		}
	}
	z = 2;
	wmove(win, ++z, 2);
	if (yes_no(win, "Mini-Insel?", 'n'))
		special = 4;
	else {
		wmove(win, ++z, 2);
		if (yes_no(win, "Nur Wasser?", 'n')) {
			special = 1;
			t = T_OCEAN;
		}
		else {
			wmove(win, ++z, 2);
			if (yes_no(win, "1-Terrain-Insel?", 'n')) {
				special = z;
				q = 0;
				wmove(win, ++z, 4);
				for (i = 0; i < MAXTERRAINS; i++) {
					sprintf(buf, "%d=%s; ", i, terrain[i].name);
					q += strlen(buf);
					if (q > SX - 20) {
						q = strlen(buf);
						z++;
						wmove(win, z, 4);
					}
					wAddstr(buf);
				}
				wrefresh(win);
				t = (char) map_input(win, 2, 3, "Terraintyp", 0, MAXTERRAINS - 1,
						  terrain_create(climate(y)));
				for (; z > special; z--) {
					wmove(win, z, 2);
					wclrtoeol(win);
					wrefresh(win);
					wmove(win, z, win->_maxx);
					waddch(win, '|');
					wrefresh(win);
				}
				special = 2;
			}
		}
	}
	wmove(win, ++z, 2);
	if (special != 1)
		chaos = (char)(yes_no(win, "Chaotischer Block?", 'n') ? 1 : 0);
	else
		chaos = 0;

	block_create(x, y, ISLANDSIZE, chaos, special, t);

	if (y < MINY)
		MINY -= 9;
	else if (y > MAXY)
		MAXY += 9;
	if (x < MINX)
		MINX -= 9;
	else if (x > MAXX)
		MAXX += 9;
	modified = 1;
}

char
GetTerrain(region * r)
{
	WINDOW *win;
	int q, y, i;
	win = newwin(8, SX - 10, 2, 2);
	wclear(win);
	wborder(win, '|', '|', '-', '-', '.', '.', '`', '\'');
	wmove(win, 0, 3);
	waddnstr(win, "< Terraform Region >", -1);
	wrefresh(win);
	q = 0;
	y = 2;
	wmove(win, y, 4);
	for (i = 0; i < MAXTERRAINS; i++) {
		sprintf(buf, "%d=%s; ", i, terrain[i].name);
		q += strlen(buf);
		if (q > SX - 20) {
			q = strlen(buf);
			y++;
			wmove(win, y, 4);
		}
		wAddstr(buf);
	}
	wrefresh(win);
	q = map_input(win, 2, 1, "Neuer Terraintyp?", -1, MAXTERRAINS - 1, r->terrain);
	if (q < 0)
		return MAXTERRAINS * 2;
	else
		return (char) q;
}

static terrain_t
choose_terrain(terrain_t t) {
	int q;

	if(rand()%100 < 50) return t;
	if(rand()%100 < 10) return T_OCEAN;

	q=rand()%100;
	switch(t) {
		case T_OCEAN:
			if(rand()%100 < 60) return T_OCEAN;
			switch(rand()%6) {
			case 0:
				return T_SWAMP;
			case 1:
				return T_PLAIN;
			case 2:
				return T_GLACIER;
			case 3:
				return T_DESERT;
			case 4:
				return T_HIGHLAND;
			case 5:
				return T_MOUNTAIN;
			}
			break;
		case T_PLAIN:
			if (q<20)
				return T_SWAMP;
			if (q<40)
				return T_MOUNTAIN;
			if (q<50)
				return T_HIGHLAND;
			if (q<80)
				return T_DESERT;
			if (q<90)
				return T_GLACIER;
			break;
		case T_SWAMP:
			if (q<40)
				return T_PLAIN;
			if (q<60)
				return T_HIGHLAND;
			if (q<80)
				return T_MOUNTAIN;
			if (q<90)
				return T_GLACIER;
			if (q<95)
				return T_DESERT;
			break;
		case T_HIGHLAND:
			if (q<35)
				return T_PLAIN;
			if (q<50)
				return T_MOUNTAIN;
			if (q<70)
				return T_GLACIER;
			if (q<80)
				return T_SWAMP;
			if (q<90)
				return T_DESERT;
			break;
		case T_DESERT:
			if (q<35)
				return T_PLAIN;
			if (q<45)
				return T_MOUNTAIN;
			if (q<50)
				return T_GLACIER;
			if (q<55)
				return T_SWAMP;
			if (q<70)
				return T_HIGHLAND;
			break;
		case T_MOUNTAIN:
			if (q<35)
				return T_PLAIN;
			if (q<65)
				return T_HIGHLAND;
			if (q<80)
				return T_GLACIER;
			if (q<90)
				return T_SWAMP;
			if (q<95)
				return T_DESERT;
			break;
		case T_GLACIER:
			if (q<35)
				return T_PLAIN;
			if (q<55)
				return T_MOUNTAIN;
			if (q<75)
				return T_HIGHLAND;
			if (q<90)
				return T_SWAMP;
			if (q<95)
				return T_DESERT;
			break;
	}
	return t;
}

region *t_queue[10000];
int t_queue_len = 0;

void
push(region *r) {
	t_queue[t_queue_len] = r;
	t_queue_len++;
}

static region *
shift(void)
{
	int p;
	region *r;

	if(t_queue_len == 0) return NULL;

	p = rand()%t_queue_len;
	r = t_queue[p];

	memmove(&t_queue[p], &t_queue[p+1], (10000-p)*sizeof(region *));
	t_queue_len--;
	return r;
}
#ifdef NEW_ITEMS
static const luxury_type * tradegood = NULL;

void
settg(region *r)
{
	const luxury_type * ltype;
	int g=0;
	if (tradegood==NULL) tradegood = luxurytypes;

	for (ltype=luxurytypes; ltype; ltype=ltype->next) {
		struct demand * dmd;
		dmd = calloc(sizeof(struct demand), 1);
		dmd->type = ltype;
		if (ltype!=tradegood) dmd->value = 1 + rand() % 5;
		++g;
	}
	r_setdemand(r, tradegood, 0);
	if (rand()%100 < 20) {
		int t = rand() % g;

		for (tradegood = luxurytypes;t;--t) {
			tradegood = tradegood->next;
		}
	}
}

#else
int tradegood;

void
settg(region *r)
{
	int i;

	for(i=0; i <= MAXLUXURIES; i++) {
		rsetdemand(r, i, 1+rand()%5);
	}
	rsetdemand(r, tradegood, 0);
	if(rand()%100 < 20) tradegood = rand()%7;
}
#endif

boolean
Create_Island(region *r, int * n, terrain_t t, int x, int y) {
	if (!r) return false;
	if (*n == 0) return true;

	if((t == T_MOUNTAIN || t == T_GLACIER) && rand()%100 < 5) {
		terraform(r,T_VOLCANO);
	} else {
		terraform(r,t);
	}
	if(r->land) settg(r);
	(*n)--;

	return false;
}

void
create_island(region *r, int n, terrain_t t) {
	int sx=r->x, sy=r->y, i, x = 0, y = 0;
	direction_t d;
	boolean abbruch=false;
	region *r2;

	for(r2=regions; r2; r2=r2->next) {
		r2->msgs = (void *)0;
	}
#ifdef NEW_ITEMS
	tradegood = NULL;
#else
	tradegood = rand()%7;
#endif
	terraform(r,t);
	if(r->land) settg(r);
	r->msgs = (void *)(rand()%6);

	push(r);
	for(d=0; d<MAXDIRECTIONS; d++) {
		region * r2 = rconnect(r, d);
		if(r2) {
			push(r2);
			r2->msgs = (void *)(int)d;
		}
	}

	while (n>0 && (r=shift())!=NULL) {
		for(i = -1; i <= 1; i++) {
			int d = ((int)r->msgs + i + 6)%6;

			switch(d) {
			case 0:
				x=r->x-1; y=r->y;		/* Westen */
				break;
			case 1:
				x=r->x-1; y=r->y+1; /* Nordwesten */
				break;
			case 2:
				x=r->x; y=r->y+1;		/* Nordosten */
				break;
			case 3:
				x=r->x+1; y=r->y;		/* Osten */
				break;
			case 4:
				x=r->x+1; y=r->y-1;	/* Südosten */
				break;
			case 5:
				x=r->x; y=r->y-1;   /* Südwesten */
				break;
			}
			r2 = findregion(x,y);
			if(r2 && r2->terrain == T_OCEAN ) {
				r2->msgs = (void *)d;
				push(r2);
				abbruch=Create_Island(r2,&n,choose_terrain(r->terrain),sx,sy);
			}
			if (abbruch) break;
		}
	}

	t_queue_len = 0;
}

