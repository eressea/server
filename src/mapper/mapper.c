/* vi: set ts=2:
 *
 *	$Id: mapper.c,v 1.10 2001/02/09 19:52:59 corwin Exp $
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

#define VERSION "3.3.0"

#define MAIN_C
#define BOOL_DEFINED
/* wenn config.h nicht vor curses included wird, kompiliert es unter windows nicht */
#include <config.h>
#include <curses.h>
#include <eressea.h>
#include "mapper.h"

#include <attributes/attributes.h>
#include <triggers/triggers.h>
#include <items/weapons.h>
#include <items/items.h>

#include <modules/xmas2000.h>
#include <modules/arena.h>
#include <modules/museum.h>

#include <item.h>
#include <faction.h>
#include <region.h>
#include <reports.h>
#include <save.h>
#include <unit.h>

#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern char *reportdir;
extern char *datadir;
extern char *basedir;
extern int maxregions;
char datafile[256];

/* -------------------- resizeterm ------------------------------------- */

short Signals = 0;
unit *clipunit;
ship *clipship;
region *clipregion;
tagregion *Tagged;

#undef NEW_DRAW
extern void render_init(void);

WINDOW * openwin(int b, int h, const char* t) {
	WINDOW * win = newwin(h,b,(SY-(h))/2,(SX-(b))/2);
	wclear(win);
	wborder(win, '|','|','-','-','.','.','`','\'');
	if(t) {
		wmove(win,0,3);
		waddnstr(win,(char*)t,-1);
	}
	wrefresh(win);
	return win;
}

#if !defined(WIN32)

#ifdef SOLARIS
#define __USE_POSIX	/* Sonst kein sigaction */
#define _POSIX_C_SOURCE 1 /* dito */
#include <sys/signal.h>
#endif

#include <signal.h>

void
sighandler(int sig)
{				/* wir fangen nur das eine Signal ab */
	Signals |= S_SIGWINCH;
}

void
signal_init(void)
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	act.sa_handler = sighandler;
	sigaction(SIGWINCH, &act, NULL);
}
#endif	/* ndef Win32 */

int left = 0, top = 0, breit, hoch, tx=-999, ty=-999;
int MINX=0, MINY=0, MAXX=0, MAXY=0;
boolean minimapx=false,minimapy=false;		/* Karte nicht vert./hor. scrollen */

/* rx,ry => Auf welcher Region steht der Cursor */
/* hoch, breit => Breite und Höhe der Karte */
/* x, y => ? */
/* top, left => ? */
/* hx, hy => ? */

void
init_win(int x, int y) {
  if (initscr() == NULL) {
    puts("\07Error initializing terminal.");
    exit(1);
  }
  keypad(stdscr, TRUE);
	scrollok(stdscr,false);
	curs_set(0);
  cbreak();
  noecho();
	clear();
	refresh();

	breit=(SX-45)/2; hoch=SY-6;

	if (breit > MAXX-MINX+(MAXY-MINY)/2) {
		left=MINX-(breit-MAXX-MINX)/2+y/2;
		breit=MAXX-MINX+(MAXY-MINY)/2;
		minimapx=true;
	} else {
		left=x-breit/2+y/2;
	}

	if (hoch > MAXY-MINY) {
		minimapy=true;
		top=MAXY+(hoch-MAXY-MINY)/2;
		hoch=MAXY-MINY;
	} else top=y+hoch/2;

	/* breit=(SX-45)/2; hoch=SY-6; */	/* zum Darstellen schon alles nehmen */
}

/* --------------------------------------------------------------------- */

static int hl;
int politkarte = 0;

chtype
RegionSymbol(region *r) {
	chtype rs;
	int p;

	if (!r) return '?';

	switch(politkarte) {
	case 1:
		{
			unit *u;
			faction *f;

			for (f = factions; f; f = f->next)
				freset(f, FL_DH);
			for (u = r->units; u; u = u->next)
				fset(u->faction, FL_DH);
			for (p = 0, f = factions; f; f = f->next)
				if (fval(f, FL_DH))
					p++;
			if (p>9)
				rs = (chtype)'!';
			else if (p>0)
				rs = (chtype)(p+48);
			else {
				rs = terrain[rterrain(r)].symbol;
				if(rs == 'P' && rtrees(r) >= 600) rs = 'F';
			}
		}
		break;
#ifdef NEW_ITEMS
		/* todo */
#else
	case 2:
		{
			int tg;

			if(r->land) {
				for(tg = 0; tg != MAXLUXURIES; tg++) {
					if(!rdemand(r, tg)) break;
				}
				rs = (unsigned char)itemdata[FIRSTLUXURY + tg].name[1][0];
			} else {
				rs = terrain[rterrain(r)].symbol;
				if(rs == 'P' && rtrees(r) >= 600) rs = 'F';
			}
		}
		break;
#endif
	case 3:
		{
			const herb_type *herb = rherbtype(r);
			if(herb) {
				const char *c = resourcename(herb2resource(herb),0);
				int h = atoi(c+1);
				if(h < 10) {
					rs = '0'+h;
				} else {
					rs = 'a'+(h-10);
				}
			} else {
				rs = terrain[rterrain(r)].symbol;
			}
		}
		break;
	case 4:
		{
			if(r->land) {
				struct demand *dmd;
				rs = '0';
				for (dmd=r->land->demands;dmd;dmd=dmd->next) rs++;
			} else {
				rs = terrain[rterrain(r)].symbol;
			}
		}
		break;
	default:
		rs = terrain[rterrain(r)].symbol;
		if(rs == 'P' && rtrees(r) >= 600) rs = 'F';
	}
	return rs;
}

boolean
is_tagged(region *r) {
	tagregion *t;

	for (t=Tagged; t; t=t->next)
		if (t->r == r)
			return true;
	return false;
}

static boolean
factionhere(region * r, int f)
{
	unit *u;
	for (u = r->units; u; u = u->next)
		if (u->faction->no == f)
			return true;
	return false;
}

void
drawmap(boolean maponly) {
	int x, x1, y1, y2, s, q;
	chtype rs;
	region *r;

	x1=left; y1=top;
	if(maponly == false) {
		movexy(SX-39, 0);
		vline('|', SY+1);
		movexy(SX-38, SY-2);
		hline('-', 39);
		if (hl != -1) {
			movexy(SX-36,SY-2);
			addstr(" High: ");
			switch (hl) {
				case -2:
					addstr("Einheiten ");
					break;
				case -3:
					addstr("Gebäude ");
					break;
				case -4:
					addstr("Schiffe ");
					break;
				case -5:
					addstr("Laen ");
					break;
				case -6:
					addstr("Mallorn ");
					break;
				case -7:
					addstr("Chaos ");
					break;
				default:
					printw((NCURSES_CONST char*)"Partei %d ",hl);
			}
		}
		switch(politkarte) {
		case 0:
			movexy(SX-14,SY-2);
			addstr(" Geol.Karte ");
			break;
		case 1:
			movexy(SX-14,SY-2);
			addstr(" Politkarte ");
			break;
		case 2:
			movexy(SX-14,SY-2);
			addstr(" Handelskar ");
			break;
		case 3:
			movexy(SX-14,SY-2);
			addstr(" Botanik    ");
			break;
		case 4:
			movexy(SX-14,SY-2);
			addstr(" Demand     ");
			break;
		}
	}

	for (s=0; s<hoch; s++) {
		movexy(1,s+6);
		printw((NCURSES_CONST char*)"%4d", y1-s);
	}

	y2=0; q=hoch; s=q+(hoch&1)+(top&1);
	do {	/* Zeilen */
		movexy(8-(s&1),y2+6);
		for (x=x1; x<x1+breit-2+(s&1); x++) {	/* Spalten */
			r  = findregion(x, y1);
			rs = RegionSymbol(r);

			addch(' ');
			if (r) {
				if ((hl == -2 && r->units) ||
					 (hl == -3 && r->buildings) ||
					 (hl == -4 && r->ships) ||
					 (hl == -5 && rlaen(r) != -1) ||
					 (hl == -6 && fval(r, RF_MALLORN)) ||
					 (hl == -7 && fval(r, RF_CHAOTIC)) ||
					 (hl >= 0 && factionhere(r, hl)) ||
					 (x==tx && y1==ty))
					addch(rs | A_REVERSE);
				else if (is_tagged(r))
					addch(rs | A_BOLD);
				else
					addch(rs);
			} else
				addch(rs);
		}
		addch(' ');
		q--; y1--; y2++; x1+=(s&1); s--;
	} while (q);

	if(maponly == false) {
		movexy(SX-37, SY-1);
		if (clipship)
			saddstr(Shipid(clipship));
		else
			addstr("kein Clipschiff");
		movexy(SX - 37, SY);
		if (clipunit)
			addstr(Unitid(clipunit));
		else
			addstr("keine Clipunit");
	}
	refresh();
}

void
mark_region(int x1, int y1, int x2, int y2)
{
	int dx, dy, q;

	x1=x1*2+7+(y1&1)+(top&1); y1+=6;
	x2=x2*2+7+(y2&1)+(top&1); y2+=6;

	dy=abs(y2-y1);
	dx=abs(x2-x1)-dy;
	if (y1<y2) {
		q=y1; y1=y2; y2=q;
		q=x1; x1=x2; x2=q;
	}
	if (x1>x2) {
		x1-=dx; x2+=dx;
	}
		/* jetzt ist <1> immer unten links, <2> immer oben rechts */

	movexy(x1-1,y1+1); hline('-', dx+2);
	movexy(x1+dy,y2-1); hline('-', dx+2);

	for (; dy>=0; dy--) {
		movexy(x1+dy-1,y1-dy);
		addch('/');
		movexy(x1+dy+dx+1,y1-dy);
		addch('/');
	}
}

void
mark(int x, int y, int rx, int ry) {
	int q;
	char num[6];


	/*
	move(0,0);
	printw("hoch=%d,breit=%d,x=%d,y=%d,left=%d,top=%d,rx=%d,ry=%d    ",
		hoch,breit,x,y,left,top,rx,ry);
	*/

	movexy(x*2+6+(y&1)+(top&1),y+6);
	if (rx==tx && ry==ty)
		addch('>');
	else
		addch('<');
	addch(RegionSymbol(findregion(rx,ry)) | A_REVERSE);
	if (rx==tx && ry==ty)
		addch('<');
	else
		addch('>');
	attron(A_BOLD);
	sprintf(num,"%4d",rx);
	for (q=0; q<5; q++) {
		movexy(x*2+7+(y&1)+(top&1),q+1);
		printw((NCURSES_CONST char*)"%c", num[q]);
	}
	movexy(1,y+6);
	printw((NCURSES_CONST char*)"%4d<-",ry);
	movexy(SX-39, y+6); addch('<');
	movexy(x*2+7+(y&1)+(top&1),5); addch('^');
	attroff(A_BOLD);
	refresh();
}

void unmark(int x, int y, int rx, int ry) {
	int q;
	region *r=findregion(rx,ry);
	chtype rs;

	rs = RegionSymbol(r);

	movexy(x*2+6+(y&1)+(top&1),y+6);
	addch(' ');
	if (r) {
		if ((hl == -2 && r->units) ||
				(hl == -3 && r->buildings) ||
				(hl == -4 && r->ships) ||
				(hl >= 0 && factionhere(r, hl)) ||
				(rx==tx && ry==ty))
			addch(rs | A_REVERSE);
		else if (is_tagged(r))
			addch(rs | A_BOLD);
		else
			addch(rs);
	} else
		addch(rs);
	addch(' ');
	for (q=0; q<5; q++) {
		movexy(x*2+7+(y&1)+(top&1),q+1);
		addch(' ');
	}
	movexy(1,y+6);
	printw((NCURSES_CONST char*)"%4d  ",ry);
	movexy(SX-39, y+6); addch('|');
	refresh();
}

void
modify_block(void)
{
	WINDOW *win;
	int c, s = 0;
	region *r;
	tagregion *t;

	win = openwin(70, 4, "< Tag-Regionen modifizieren >");
	wmove(win, 1, 2);
	wAddstr("Bauern, Pferde, Silber, Chaosstatus (b/p/s/c,q)?");
	wrefresh(win);
	c = getch();
	if (c == 'q') {
		delwin(win);
		return;
	}
	if (c == 'c') {
		wmove(win, 2, 2);
		wAddstr("Chaosstatus (s)etzen oder (l)öschen?");
		wrefresh(win);
		if (getch() == 's') {
			s = 1;
		} else {
			s = 0;
		}
	  c = 'c';
	}

	for (t=Tagged; t; t=t->next) {
		r=t->r;
		if (production(r)) {
			switch (c) {
				case 'b':
					rsetpeasants(r, production(r)*3+rand()%(production(r)*3));
					break;
				case 'p':
					rsethorses(r, rand()%(production(r) / 10));
					break;
				case 's':
					rsetmoney(r, production(r)*10+rand()%(production(r)*10));
					break;
			}
		}
		if (c == 'c') {
			if(s) {
				fset(r, RF_CHAOTIC);
			} else {
				freset(r, RF_CHAOTIC);
			}
		}
	}
	modified = 1;
	delwin(win);
}

void
SetHighlight(void)
{
	WINDOW *win;
	char *fac_nr36;
	int c;
	win = openwin(60, 5, "< Highlighting >");
	wmove(win, 1, 2);
	wAddstr("Regionen mit P)artei, E)inheiten, B)urgen, S)chiffen,");
	wmove(win, 2, 2);
	wAddstr("             L)aen, C)haos oder N)icht Highlighten?");
	wrefresh(win);
	c = tolower(getch());
	switch (c) {
	case 'p':
		fac_nr36 = my_input(win, 2, 2, "Partei-Nummer: ", NULL);
		hl = atoi36(fac_nr36);
		break;
	case 'e':
		hl = -2;
		break;
	case 'b':
		hl = -3;
		break;
	case 's':
		hl = -4;
		break;
	case 'l':
		hl = -5;
		break;
	case 'm':
		hl = -6;
		break;
	case 'c':
		hl = -7;
		break;
	case 'n':
	default:
		hl = -1;
		break;
	}
	delwin(win);
}

void
recalc_everything(int *x, int *y, int *rx, int *ry)
{
	int eingerueckt;

	top=(*ry)+hoch/2; (*y)=hoch/2;
	(*x)=breit/2; left=(*rx)-((*y)/2)-(*x);

	/* Wenn Zeile eingerückt, left modifizieren */

	if((top-(*ry))&1) /* Einrückung wie erste Zeile */
		eingerueckt = (hoch + (hoch&1) + (top&1))&1;
	else
		eingerueckt = !((hoch + (hoch&1) + (top&1))&1);

	if(eingerueckt) left++;
}

#define restore { x=oldx; y=oldy; rx=oldrx; ry=oldry; }

void
movearound(int rx, int ry) {
	int hx = -1, hy = -1, ch, x, y, Rand, d, a, b, p, q, oldx=0, oldy=0;
	int oldrx, oldry, Hx=0, Hy=0;
	int sel;
	char *selc;
	region *c, *r = NULL, *r2;
	tagregion *tag;
	unit *u, *u2;
	WINDOW *win;

	y=top-ry; x=rx-left; left-=y/2;
	if (y&1 && !(ry&1)) left--; else left++;
	if (ry&1) left--;

	r=findregion(rx,ry);
	drawmap(false);
	Rand=min(4, min(breit/4, hoch/4));
	showregion(r, 0);
	DisplayRegList(1);
	mark(x,y,rx,ry);

	for (;;) {
#ifndef WIN32
		if (Signals & S_SIGWINCH) {
			do {
				endwin();
				init_win(rx, ry);
				Signals &= ~S_SIGWINCH;
				if (SX < 50 || SY < 12) {
					beep();
					move(0, 0);
					addstr("Fenster zu klein!");
					refresh();
					while (!(Signals & S_SIGWINCH))
						a++;
				}
			} while (SX < 50 && SY < 12);
			y = top - ry; x = rx - left; left-=y/2;
			/* Wenn in eingerückter Zeile */
			if (y&1 && !(ry&1)) left--; else left++;
			if (ry&1) left--;
			ch = -9;
		} else
#endif
		{
			ch = getch();
			unmark(x, y, rx, ry);

			oldx=x; oldy=y; oldrx=rx; oldry=ry;
			switch (ch) {
				case KEY_HELP:
				case '?':
					if (reglist) {
						freelist(reglist);
						reglist = NULL;
					}
					adddbllist(&reglist, "Hilfe");
					adddbllist(&reglist, "¯¯¯¯¯");
					adddbllist(&reglist, " Cursor: Scroll");
					adddbllist(&reglist, " NumBlock: Move");
					adddbllist(&reglist, " j,m,x,y: Jump X/Y");
					adddbllist(&reglist, " ^L: Redraw");
					adddbllist(&reglist, " h: Bereich markieren");
					adddbllist(&reglist, " p: Parteiliste");
					adddbllist(&reglist, " e,u: Einheitenliste");
					adddbllist(&reglist, " q: Quit");
					adddbllist(&reglist, " r,i: Regionsinfo");
					adddbllist(&reglist, " t: Tag Region(en)");
					adddbllist(&reglist, " ^T: Untag alle Regionen");
					adddbllist(&reglist, " /: Suche Einheit");
					adddbllist(&reglist, " B: Ozean-Bereich erzeugen");
					adddbllist(&reglist, " ^B: neuen 9×9-Block erzeugen");
					adddbllist(&reglist, " C: neue Region erzeugen");
					adddbllist(&reglist, " D: Partei löschen");
					adddbllist(&reglist, " G: Jump Tagregion");
					adddbllist(&reglist, " H: Highlight");
					adddbllist(&reglist, " M: Region(en) modifizieren");
					adddbllist(&reglist, " P: neue Partei einfügen");
					adddbllist(&reglist, " T: Terraform Region");
					adddbllist(&reglist, " Z: Terrain Region");
					DisplayRegList(1);
					ch = 999999;
					break;
				case 'q':
					if (yes_no(0, "Wirklich beenden?", 'n'))
						return;
					/* sonst ein redraw */
				case 12:	/* ^L */
					clear();
					DisplayRegList(0);
					ch = -9;	/* nur ein Redraw */
					break;
				case 'T':
					if (!Tagged) {
						if (hx>-1) {
							int Rx,Ry;
							Rx=rx; Ry=ry;
							if (rx>Hx) { a=Hx; Hx=Rx; rx=a; }
							if (ry>Hy) { a=Hy; Hy=Ry; ry=a; }
							ch = GetTerrain(r);
							for (a=Rx; a<=Hx; a++)
								for (b=Ry; b<=Hy; b++) {
									c=findregion(a,b);
									if (c) terraform(c, (terrain_t) ch);
								}
						} else
							terraform(r, GetTerrain(r));
					} else {
						ch = GetTerrain(r);
						for (tag=Tagged; tag; tag=tag->next)
							terraform(tag->r, (terrain_t) ch);
					}
					ch = -9;
					modified = 1;
					break;
				case 'Z':
					if (!Tagged) {
						if (hx>-1) {
							int Rx,Ry;
							Rx=rx; Ry=ry;
							if (rx>Hx) { a=Hx; Hx=Rx; rx=a; }
							if (ry>Hy) { a=Hy; Hy=Ry; ry=a; }
							ch = GetTerrain(r);
							for (a=Rx; a<=Hx; a++)
								for (b=Ry; b<=Hy; b++) {
									c=findregion(a,b);
									if (c) rsetterrain(c, (terrain_t) ch);
								}
						} else
							terraform(r, GetTerrain(r));
					} else {
						ch = GetTerrain(r);
						for (tag=Tagged; tag; tag=tag->next)
							rsetterrain(tag->r, (terrain_t) ch);
					}
					ch = -9;
					modified = 1;
					break;
				case 't':
					if (hx>-1) {
						if (Hx<rx) { a=Hx; Hx=rx; rx=a; }
						if (Hy<ry) { a=Hy; Hy=ry; ry=a; }
						for (a=rx; a<=Hx; a++)
							for (b=ry; b<=Hy; b++) {
								c=findregion(a,b);
								if (!c) continue;
								if (!is_tagged(c)) {
									tag=calloc(1, sizeof(tagregion));
									tag->r=c;
									tag->next=Tagged; Tagged=tag;
								}
							}
						ch=-9;
					} else {
						tx=rx; ty=ry;
						if (is_tagged(r)) {
							tag=Tagged;
							while (tag->r!=r) tag=tag->next;
							removelist(&Tagged,tag);
						} else {
							tag=calloc(1, sizeof(tagregion));
							tag->r=r;
							tag->next=Tagged; Tagged=tag;
						}
					}
					break;
				case 0x14:
					{
						tagregion *tag_next;
						tag = Tagged;
						while (tag) {
							tag_next = tag->next;
							removelist(&Tagged, tag);
							tag = tag_next;
						}
						ch=-9;
					} break;
				case 'G':
					rx=tx; ry=ty;
					recalc_everything(&x, &y, &rx, &ry);
					ch=-8;
					break;
				case 'c':
					if (clipunit) {
						rx = clipregion->x;
						ry = clipregion->y;
						recalc_everything(&x, &y, &rx, &ry);
						ch = -8;
					}
					break;
				case '/':
					win = openwin(70, 4, "< Suchen >");
					wmove(win, 1, 2);
					wAddstr("Suchen nach P)artei, E)inheit, R)egionsname? ");
					wrefresh(win);
					sel = tolower(getch());
					switch(sel) {
					case 'e':
					case 'E':
						q = atoi36(my_input(win, 2, 2, "Welche Einheit suchen: ", NULL));
						if (q) {
							u = findunitg(q, NULL);
							if (!u) {
								warnung(0, "Einheit nicht gefunden.");
							} else {
								r = u->region;
								rx=r->x; ry=r->y;
								recalc_everything(&x, &y, &rx, &ry);
							}
						}
						break;
					case 'p':
					case 'P':
						q = atoi36(my_input(win, 2, 2, "Welche Partei suchen: ", NULL));
						if(q) {
							u2 = NULL;
							for(r2=regions; r2; r2=r2->next) {
								for(u2=r2->units; u2; u2=u2->next)
									if(u2->faction->no == q) break;
								if(u2) break;
							}

							if(u2) {
								r = r2;
								rx=r->x; ry=r->y;
								recalc_everything(&x, &y, &rx, &ry);
							}
						}
						break;
					case 'r':
					case 'R':
						selc = my_input(win, 2, 2, "Welchen Regionsnamen suchen: ", NULL);
						if(*selc) {
							for(r2=regions; r2; r2=r2->next)
								if(strcmp(selc, rname(r2, NULL)) == 0) break;

							if(r2) {
								r = r2;
								rx=r->x; ry=r->y;
								recalc_everything(&x, &y, &rx, &ry);
							}
						}
						break;
					}

					delwin(win);
					ch=-9;
					break;
				case 'H':
					SetHighlight();
					ch = -9;
					break;
				case 'h':
					if (hx < 0) {
						hx = x; Hx=rx;
						hy = y; Hy=ry;
					} else
						hx = hy = -1;
					ch=-9;
					break;
				case 'C':
					make_new_region(rx, ry);
					ch = -9;
					break;
				case 'B':
					a=map_input(0,0,0,"Start X", -999, 999, rx);
					b=map_input(0,0,0,"Start Y", -999, 999, ry);
					p=map_input(0,0,0,"Ende X", -999, 999, rx);
					q=map_input(0,0,0,"Ende Y", -999, 999, ry);
					if (a>p) { x=a; a=p; p=x; }
					if (b>q) { x=b; b=q; q=x; }
					for (x=a; x<=p; x++)
						for (y=b; y<=q; y++)
							if (!findregion(x,y))	/* keine existierenden Regionen überschreiben */
								(void)new_region(x,y);
					restore;
					modified=1;
					ch=-9;
					break;
				case 'I':
					a=map_input(0,0,0,"Wieviele Regionen?",0,500,0);
					if (a) {
						create_island(r, a, (terrain_t)(rand()%(T_GLACIER)+1));
						modified=1;
					}
					ch = -9;
					break;
				case 0x2:	/* die altmodische Art */
					make_new_block(rx, ry);
					ch = -9;
					break;
				case 'S':
					if (modified)
						if (yes_no(0, "Daten abspeichern?", 'j')) {
							remove_empty_units();
							writegame(datafile, 1);
							if (yes_no(0, "Backup neu anlegen?", 'j')) {
								create_backup(datafile);
							}
							modified = 0;
							ch = -8;
						}
					break;
				case 'D':
					RemovePartei();
					ch = -9;
					break;
				case 's':
					c = SeedPartei();
					if (c) {
						rx = c->x;
						ry = c->y;
						recalc_everything(&x, &y, &rx, &ry);
						ch = -8;
					}
					break;
				case 'P':
					NeuePartei(r);
					ch = -9;
					break;
				case 'M':
					if (Tagged)
						modify_block();
					else
						while (modify_region(r));	/* Liste neu bei einigen Aktionen */
					ch = -9;
					break;
				case 'u':
				case 'e':
					while (showunits(r));
					ch = -9;
					break;
				case 'E':
					clipunit = 0;
					clipregion = 0;
					break;
				case 'i':
				case 'r':
					showregion(r, 1);
					DisplayRegList(1);
					ch = 999999;
					break;
				case 'p':
					while (ParteiListe());
					ch = -9;
					break;
				case 'k':
					politkarte = (politkarte+1)%5;
					ch = -9;
					break;
				case '{':
				case KEY_PPAGE:
					ScrollRegList(1);
					ch = 999999;
					break;
				case '}':
				case KEY_NPAGE:
					ScrollRegList(-1);
					ch = 999999;
					break;
				case 'j':
				case 'g':
					if (hx<0) {
						rx=map_input(0,0,0,"Neue X-Koordinate", MINX, MAXX, rx);
						ry=map_input(0,0,0,"Neue Y-Koordinate", MINY, MAXY, ry);

						recalc_everything(&x, &y, &rx, &ry);

						ch=-8;
					}
					break;
				case '1':	/* left down */
					y++; ry--;
					if (y&1) x--;
					break;
				case '7':	/* left up */
					y--; ry++;
					if (!(y&1)) x++;
					x--; rx--;
					break;
				case '3':	/* right down */
					rx++;
					if (y&1) x++;
					y++; ry--;
					break;
				case '9':	/* right up */
					y--; ry++;
					if (!(y&1)) x++;
					break;
				case '2':
					y++; ry--;
					if (y&1) rx++;
					break;
				case '8':
					y--; ry++;
					if (!(y&1)) rx--;
					break;
				case '4':
					x--; rx--;
					break;
				case '6':
					x++; rx++;
					break;
				case '5':
					if (hx<0) {
						top=ry+hoch/2; y=hoch/2;
						x=breit/2; left=rx-(y/2)-x; if (top&1) left--;
						if (!(hoch&1)) left++;
						ch=-8;
					} else {
						ch=0;
						beep();
					}
					break;
				case '[':
					left++;
					ch=-9;
					break;
				case ']':
					left--;
					ch=-9;
					break;
				case KEY_LEFT:	/* scroll left */
					sprintf(buf, "%x", ch);
					mvaddnstr(0, 0, buf, 4);
					if (hx<0 && !minimapx) {
						ch=-2;
						rx--; left--;
					} else {
						ch=0;
						beep();
					}
					break;
				case KEY_RIGHT:	/* scroll right */
					if (hx<0 && !minimapx) {
						ch=-3;
						rx++; left++;
					} else {
						ch=0;
						beep();
					}
					break;
				case KEY_DOWN:	/* scroll down */
					if (hx<0 && !minimapy) {
						ch=-4;
						top--;
						if ((top&1)) left--; else left++;
					} else {
						ch=0;
						beep();
					}
					break;
				case KEY_UP:	/* scroll up */
					if (hx<0 && !minimapy) {
						ch=-5;
						top++;
						if (!(top&1)) left++; else left--;
					} else {
						ch=0;
						beep();
					}
					break;
			}
		}

		if (ch>0) {
			if (!minimapx) {
				if (x<Rand-1 && left > MINX) {
					if (hx>-1) restore
					else {
						left-=(breit-2*Rand+2);
						x=breit-Rand;
						ch=-1;
					}
				} else if (x > breit-Rand && left+breit < MAXX) {
					if (hx>-1) restore
					else {
						left+=(breit-2*Rand+1);
						x=Rand;
						ch=-1;
					}
				}
			}
			if (!minimapy) {
				if (y < Rand-1 && top < MAXY) {
					if (hx>-1) restore
					else {
						d=(hoch-2*Rand+2);
						top+=d;
						left-=d/2;
						if (hoch&1) left--;
						else if (ry&1) left--; else left++;
						if (ry&1) left++; else left--;
						y=hoch-Rand;
						ch=-1;
					}
				} else if (y > hoch-Rand && top-hoch >= MINY) {
					if (hx>-1) restore
					else {
						d=(hoch-2*Rand+2);
						top-=d;
						left+=d/2;
						if (!(hoch&1)) left++;
						if (ry&1) {
							if (hoch&1) left++; else left--;
						} else left--;
						y=Rand;
						ch=-1;
					}
				}
			}
		}

		if (ch < 0) {
			if (ch > -9) {	/* sonst sind wir an alter Position */
				ry=top-y;
			}
			if (ch == -9) {
				clear();
				if (!r || r->x != rx || r->y != ry)
					r=findregion(rx,ry);
				showregion(r, 1);
				DisplayRegList(1);
			}
			drawmap(false);
		}
		if (!r || r->x != rx || r->y != ry)
			r=findregion(rx,ry);
		if (ch < 999999 && ch > -9) {	/* Lange Regionsinfo nicht überschreiben */
			showregion(r, 0);
			DisplayRegList(1);
		}
		if (hx>-1 && (x!=oldx || y!=oldy)) {
			if (ch>0) drawmap(true);	/* sonst war das eben schon */
			mark_region(x, y, hx, hy);
		}
		mark(x,y,rx,ry);
	}
}

void
Exit(int level)
{
	move(SY, 0);
	deleteln();
	refresh();
	curs_set(1);
	endwin();
	exit(level);
}


void
usage(void)
{
	fprintf(stderr, "mapper [-b basedir] [-d datadir] [-o datafile] [-c x,y]\n");
	exit(0);
}

void
setminmax(void)
{
	region *r;
	for (r = regions; r; r = r->next) {
		if (r->x < MINX)
			MINX = r->x;
		if (r->y < MINY)
			MINY = r->y;
		if (r->x > MAXX)
			MAXX = r->x;
		if (r->y > MAXY)
			MAXY = r->y;
	}
	MAXX += 20;
	MAXY += 20;
	MINX -= 20;
	MINY -= 20;
}

extern boolean quiet;

extern char * g_reportdir;
extern char * g_datadir;
extern char * g_basedir;

int
main(int argc, char *argv[])
{
	int x = 0, y = 0, i;
	char *s;
	boolean backup = true;

	setlocale(LC_ALL, "");

	fprintf(stderr, "\n	Mapper V" VERSION " Hex (" __DATE__ ")\n\n"
		"Copyright © 1998/99 by Henning Peters  -  faroul@gmx.de\n"
		"Copyright © 1998 by Enno Rehling  -  enno.rehling@gmx.de\n"
		"Copyright © 2000 by Christian Schlittchen  -  corwin@amber.kn-bremen.de\n\n");

	*datafile = 0;

	if (argc > 1) {
		if (argv[1][0] == '?')
			usage();

		i = 1;
		while (i < argc && argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 't':
				if (argv[i][2])
					turn = atoi((char *)(argv[i] + 2));
				else
					turn = atoi(argv[++i]);
				break;
			case 'x':
				maxregions = atoi(argv[++i]);
				maxregions = (maxregions*81+80) / 81;
				break;
			case 'q': quiet = true; break;
			case 'n':
				switch (argv[i][2]) {
				case 'b' : backup = false; break;
				}
				break;
			case 'd':
				g_datadir = argv[++i];
				break;
			case 'o':
				strcpy(datafile, argv[++i]);
				break;
			case 'b':
				g_basedir = argv[++i];
				break;
			case 'c':
				if (argv[i][2])
					s = strdup((char *) (argv[i] + 2));
				else
					s = argv[++i];
				x = atoi(s);
				s = strchr(s, ',');
				y = atoi(++s);
				break;
			default:
				usage();
			}
			i++;
		}
	}

	kernel_init();

	init_triggers();
	init_attributes();

	init_resources();
	/* init_weapons(); */
	init_items();

	init_museum();
	init_arena();
	init_xmas2000();

	if(!*datafile)
		sprintf(datafile, "%s/%d", datapath(), turn);

	readgame(backup);

#ifdef OLD_ITEMS
	make_xref();
#endif
	setminmax();
	srand(time((time_t *) NULL));

#ifndef WIN32
	signal_init();
#endif
	init_win(x, y);

	hl=-1;
	Tagged=NULL;
	movearound(x, y);

	if (modified) {
		beep();
 		if (yes_no(0, "Daten wurden modifiziert! Abspeichern?", 'j')) {
			remove_empty_units();
			writegame(datafile, 1);
		}
	}
	Exit(0);
	return 0;
}
