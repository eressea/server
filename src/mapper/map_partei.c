/* vi: set ts=2:
 *
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
#include <config.h>
#include <curses.h>
#include <eressea.h>
#include "mapper.h"

/* kernel includes */
#include <faction.h>
#include <plane.h>
#include <race.h>
#include <region.h>
#include <reports.h>
#include <study.h>
#include <unit.h>

/* util includes */
#include <base36.h>
#include <language.h>

/* libc includes */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void
RemovePartei(void) {
	faction *f, *F;
	ally *a, *anext;
	WINDOW *win;
	char *fac_nr36;
	int x;
	unit *u;
	region *r;
	win = openwin(SX - 20, 5, "< Partei löschen >");

	fac_nr36 = my_input(win, 2, 1, "Partei Nummer: ", NULL);

	x = atoi36(fac_nr36);

	if (fac_nr36 && *fac_nr36 && x > 0) {
		wmove(win, 1, 2);
		wclrtoeol(win);
		wrefresh(win);
		wmove(win, 1, win->_maxx);
		waddch(win, '|');
		wrefresh(win);
		F = findfaction(x);
		wmove(win, 1, 2);
		if (F) {
			wAddstr(factionname(F));
			wrefresh(win);
			wmove(win, 3, 4);
			wattron(win, A_BOLD);
			wAddstr("Alle Einheiten gehen verloren!");
			wattroff(win, A_BOLD);
			wmove(win, 3, 4);
			beep();
			if (yes_no(win, "Diese Partei wirklich löschen?", 'n')) {
				for (f = factions; f; f = f->next)
					if (f != F)
						for (a = f->allies; a; a = anext) {
							anext = a->next;
							if (a->faction->no == F->no)
								removelist(&f->allies, a);
						}
				for (r = firstregion(F); r != lastregion(F); r = r->next)
					for (u = r->units; u; u = u->next)
						if (u->faction == F)
							set_number(u, 0);
				modified = 1;
				remove_empty_units();
				removelist(&factions, F);
				findfaction(-1);	/* static lastfaction auf NULL setzen */
			}
		} else {
			wprintw(win, (NCURSES_CONST char*)"Partei %d gibt es nicht.", x);
			wmove(win, 3, 6);
			wAddstr("<Taste>");
			wrefresh(win);
			getch();
		}
	}
	delwin(win);
}

extern int left, top, breit, hoch;

typedef struct island {
	vmap factions;
	int maxage;
	int x;
	int y;
	unsigned int land;
	int age;
} island;

static region *
goodregion(int trace)
{
#define AGEGROUP 3				/* In diesem Alter wird zusammengefasst */
#define REGPERFAC 3				/* #Regionen pro faction auf einer insel */
	region *r;
	vmap *xmap = (vmap *) calloc(1, sizeof(vmap));
	cvector islands;
	void **i;
	int maxage = 0;
	island *best = 0;
	int maxscore = 1000;
	region *good = 0;

	cv_init(&islands);
	for (r = regions; r != 0; r = r->next) {
		int x = (r->x + 999) / 9;
		int y = (r->y + 999) / 9;
		unsigned int xp = vmap_find(xmap, x);

		if (xp == xmap->size) {
			xp = vmap_insert(xmap, x, calloc(1, sizeof(vmap)));
		} {
			vmap *ymap = xmap->data[xp].value;
			unsigned int yp = vmap_find(ymap, y);
			island *is;
			unit *u;

			if (yp == ymap->size) {
				is = calloc(1, sizeof(island));
				is->x = x;
				is->y = y;
				cv_pushback(&islands, is);
				yp = vmap_insert(ymap, y, is);
			} else
				is = ymap->data[yp].value;
			if (rterrain(r) != T_OCEAN && rterrain(r) != T_GLACIER)
				++is->land;

			for (u = r->units; u != 0; u = u->next) {
				faction *f = u->faction;
				unsigned int pos = vmap_find(&is->factions, f->no);

				if (pos == is->factions.size) {
					int age = 1 + (f->age / AGEGROUP);

					vmap_insert(&is->factions, f->no, f);
					is->maxage = max(is->maxage, age);
					is->age += age;
					maxage = max(maxage, is->maxage);
				}
			}
		}
	}

	for (i = islands.begin; i != islands.end; ++i) {
		island *is = *i;

		if (is->maxage > 6 || is->factions.size == 0)
			continue;
		if (is->land / REGPERFAC < is->factions.size)
			continue;
		if (is->age < maxscore) {
			best = is;
			maxscore = is->age;
		}
	}

	if (!best) {
		maxscore = 0;
		for (i = islands.begin; i != islands.end; ++i) {
			island *is = *i;
			int x, y;
			int score = 0;

			if (is->land == 0 || is->age != 0)
				continue;
			/* insel leer. nachbarn zählen. */
			for (x = is->x - 1; x <= is->x + 1; ++x) {
				unsigned int p = vmap_find(xmap, x);
				vmap *ymap;

				if (p == xmap->size)
					continue;
				ymap = xmap->data[p].value;
				for (y = is->y - 1; y <= is->y + 1; ++y) {
					unsigned int p = vmap_find(ymap, y);
					island *os;

					if (p == ymap->size)
						continue;
					os = ymap->data[p].value;
					if (os->age > 0)
						score += maxage - os->maxage;
				}
			}
			if (score > maxscore) {
				maxscore = score;
				best = is;
			}
		}
	}
	if (best) {
		int a = best->x * 9 - 999;
		int b = best->y * 9 - 999;
		int x, y;

		maxscore = 0;
		for (x = a; x != a + 9; ++x) {
			for (y = b; y != b + 9; ++y) {
				region *r = findregion(x, y);
				int score = 0;

				if (r && r->units == 0 && rterrain(r) != T_OCEAN && rterrain(r) != T_GLACIER) {
					direction_t con;
					for (con = 0; con != MAXDIRECTIONS; ++con) {
						region *c = rconnect(r, con);

						if (!c || c->units == 0)
							score += 2;
						if (c && rterrain(c) != T_OCEAN)
							++score;
					}
					if (score > maxscore) {
						maxscore = score;
						good = r;
					}
				}
			}
		}
	}
	/* cleanup */  {
		void **i;
		unsigned int x;

		for (i = islands.begin; i != islands.end; ++i) {
			free(*i);
		}
		for (x = 0; x != xmap->size; ++x) {
			vmap *v = xmap->data[x].value;
			free(v->data);
			free(v);
		}
		free(xmap->data);
		free(xmap);
	}
	return good;
}

region*
SeedPartei(void)
{
	int i, q=0, y=3;
	WINDOW *win;
	race_t rc = 0;

	do {
		win = openwin(SX - 10, 6, "< Neue Partei einfügen >");
		wmove(win, y, 4);
		for (i = 1; i < MAXRACES; i++) if(!race[i].nonplayer) {
			sprintf(buf, "%d=%s; ", i, race[i].name[0]);
			q += strlen(buf);
			if (q > SX - 20) {
				q = strlen(buf);
				y++;
				wmove(win, y, 4);
			}
			wAddstr(buf);
		}
		rc = (race_t) map_input(win, 2, 1, "Rasse", 0, MAXRACES-1, rc);

		delwin(win);
	} while(race[i].nonplayer);
	return goodregion(rc);
}

void
NeuePartei(region * r)
{
	int i, q, y;
	WINDOW *win;
	char email[INPUT_BUFSIZE+1];
	race_t frace;
	int late;
	unit *u;
	int locale_nr;

	if (!r->land) {
		warnung(0, "Ungeeignete Region! <Taste>");
		return;
	}
	win = openwin(SX - 10, 12, "< Neue Partei einfügen >");

	strcpy(buf, my_input(win, 2, 1, "EMail-Adresse (Leer->Ende): ", NULL));
	if (!buf[0]) {
		delwin(win);
		return;
	}

	strcpy(email, buf);

	y = 3;
	q = 0;
	wmove(win, y, 4);
	for (i = 0; i < MAXRACES; i++) if(!race[i].nonplayer) {
	  sprintf(buf, "%d=%s; ", i, race[i].name[0]);
	  q += strlen(buf);
	  if (q > SX - 20) {
			q = strlen(buf);
			y++;
			wmove(win, y, 4);
	  }
	  wAddstr(buf);
	}
	wrefresh(win);

	y++;
	do {
		frace = (char) map_input(win, 2, y, "Rasse", -1, MAXRACES-1, 0);
	} while(race[frace].nonplayer);
	if(frace == -1) {
		delwin(win);
		return;
	}

	y++;
	late = (int) map_input(win, 2, y, "Startbonus", -1, 99, 0);
	if(late == -1) {
		delwin(win);
		return;
	}

	i = 0; q = 0; y++;
	wmove(win, y, 4);
	while(locales[i] != NULL) {
		sprintf(buf, "%d=%s; ", i, locales[i]);
		q += strlen(buf);
		if (q > SX - 20) {
			q = strlen(buf);
			y++;
			wmove(win, y, 4);
		}
		wAddstr(buf);
		i++;
	}
	wrefresh(win);

	y++;
	locale_nr = map_input(win, 2, 8, "Locale", -1, i-1, 0);
	if(locale_nr == -1) {
		delwin(win);
		return;
	}

	delwin(win);
	modified = 1;

	u = addplayer(r, email, frace, find_locale(locales[locale_nr]));

	if(late) give_latestart_bonus(r, u, late);

#ifndef AMIGA
	sprintf(buf, "newuser %s", email);
	system(buf);
#endif
}

static void
ModifyPartei(faction * f)
{
	int c, i, q, y;
	int fmag = count_skill(f, SK_MAGIC);
	int falch = count_skill(f, SK_ALCHEMY);
	WINDOW *win;
	unit *u;
	sprintf(buf, "<%s >", factionname(f));
	win = openwin(SX - 6, 12, buf);
	wmove(win, 1, 2);
	wprintw(win, (NCURSES_CONST char*)"Rasse: %s", race[f->race].name[1]);
	wmove(win, 2, 2);
	wprintw(win, (NCURSES_CONST char*)"Einheiten: %d", f->nunits);
	wmove(win, 3, 2);
	wprintw(win, (NCURSES_CONST char*)"Personen: %d", f->num_people);
	wmove(win, 4, 2);
	wprintw(win, (NCURSES_CONST char*)"Silber: %d", f->money);
	wmove(win, 5, 2);
	wprintw(win, (NCURSES_CONST char*)"Magier: %d", fmag);
	if (fmag) {
		region *r;
		freset(f, FL_DH);
		waddnstr(win, " (", -1);
		for (r = firstregion(f); r != lastregion(f); r = r->next)
			for (u = r->units; u; u = u->next)
				if (u->faction == f && get_skill(u, SK_MAGIC)) {
					if (fval(f, FL_DH))
						waddnstr(win, ", ", -1);
					wprintw(win, (NCURSES_CONST char*)"%s(%d): %d", unitid(u), u->number, get_skill(u, SK_MAGIC) / u->number);
					fset(f, FL_DH);
				}
		waddch(win, ')');
	}
	wmove(win, 6, 2);
	wprintw(win, (NCURSES_CONST char*)"Alchemisten: %d", falch);
	if (falch) {
		region *r;
		waddnstr(win, " (", -1);
		freset(f, FL_DH);
		for (r = firstregion(f); r != lastregion(f); r = r->next)
			for (u = r->units; u; u = u->next)
				if (u->faction == f && get_skill(u, SK_ALCHEMY)) {
					if (fval(f, FL_DH))
						waddnstr(win, ", ", -1);
					wprintw(win, (NCURSES_CONST char*)"%s(%d): %d", unitid(u), u->number, get_skill(u, SK_ALCHEMY) / u->number);
					fset(f, FL_DH);
				}
		waddch(win, ')');
	}
	wmove(win, 7, 2);
	wprintw(win, (NCURSES_CONST char*)"Regionen: %d", f->nregions);
	wmove(win, 8, 2);
	wprintw(win, (NCURSES_CONST char*)"eMail: %s", f->email);
	wmove(win, 10, 2);
	wprintw(win, (NCURSES_CONST char*)"Neue (R)asse oder (e)Mail, (q)uit");
	wrefresh(win);

	for(;;) {
		c = getch();
		switch (tolower(c)) {
		case 'q':
		case 27:
			delwin(win);
			touchwin(stdscr);
			return;
		case 'r':
			mywin = newwin(5, SX - 20, (SY - 5) / 2, (SX - 20) / 2);
			wclear(mywin);
			wborder(mywin, '|', '|', '-', '-', '.', '.', '`', '\'');
			Movexy(3, 0);
			Addstr("< Rasse wählen >");
			wrefresh(mywin);
			y = 2;
			q = 0;
			wmove(mywin, y, 4);
			for (i = 1; i < MAXRACES; i++) {
				sprintf(buf, "%d=%s; ", i, race[i].name[0]);
				q += strlen(buf);
				if (q > SX - 25) {
					q = strlen(buf);
					y++;
					wmove(mywin, y, 4);
				}
				wAddstr(buf);
			}
			q = map_input(mywin, 2, 1, "Rasse", 1, MAXRACES-1, f->race);
			delwin(mywin);
			touchwin(stdscr);
			touchwin(win);
			if (q != f->race) {
				modified = 1;
				f->race = (char) q;
				wmove(win, 1, 2);
				wprintw(win, (NCURSES_CONST char*)"Rasse: %s", race[f->race].name[1]);
			}
			refresh();
			wrefresh(win);
			break;
		case 'e':
			strcpy(buf, my_input(0, 0, 0, "Neue eMail: ", NULL));
			touchwin(stdscr);
			touchwin(win);
			if (strlen(buf)) {
				strcpy(f->email, buf);
				wmove(win, 8, 2);
				wprintw(win, (NCURSES_CONST char*)"eMail: %s", f->email);
				modified = 1;
			}
			refresh();
			wrefresh(win);
			break;
		}
	}
}


int
ParteiListe(void)
{
	dbllist *P = NULL, *oben, *unten, *tmp;
	faction *f;
	char *s;
	int x;

	clear();
	move(1, 1);
	addstr("generiere Liste...");
	refresh();

	/* Momentan unnötig und zeitraubend */
#if 0
	for (f = factions; f; f = f->next) {
		f->num_people = f->nunits = f->nregions = f->money = 0;
	}

	x=0;
	for (r = regions; r; r = r->next) {
		int q=0; char mark[]="_-¯-";
		for (f = factions; f; f = f->next)
			freset(f, FL_DH);
		if (++x & 256) {
			x=0; move(20,1); q++; addch(mark[q&3]);
		}
		for (u = r->units; u; u = u->next) {
			if (u->faction->no != MONSTER_FACTION) {	/* Monster nicht */
				if (!fval(u->faction, FL_DH)) {
					fset(u->faction, FL_DH);
					u->faction->nregions++;
				}
				u->faction->nunits++;
				u->faction->num_people += u->number;
				u->faction->money += get_money(u);
			}
		}
	}
#endif

	for (f = factions->next; f; f = f->next) {
	  if (SX > 104)
		sprintf(buf, "%4s: %-30.30s %-12.12s %-24.24s", factionid(f),
				f->name, race[f->race].name[1], f->email);
	  else
		sprintf(buf, "%4s: %-30.30s %-12.12s", factionid(f),
				f->name, race[f->race].name[1]);
	  adddbllist(&P, buf);
	}

	oben = unten = pointer = P;
	pline = 0;
	x = -1;
	clear();

	movexy(0, SY - 1);
	hline('-', SX + 1);
	movexy(0, SY);
	addstr("<Ret> Daten; /: suchen; D: Löschen; q,Esc: Ende");

	for (;;) {
		if (x == -1) {
			clear();
			if(oben) {
			  for (unten = oben, x = 0; x < SY - 1 && unten->next; x++) {
				movexy(3, x);
				addstr(unten->s);
				unten = unten->next;
			  }
			}
			if (x < SY - 1 && unten) {	/* extra, weil sonst
										 * unten=NULL nach for() */
			  movexy(3, x);
			  addstr(unten->s);
			}
			movexy(0, SY - 1);
			hline('-', SX + 1);
			movexy(0, SY);
			addstr("<Ret> Daten; /: suchen; D: Löschen; q,Esc: Ende");
		}
		movexy(0, pline);
		addstr("->");
		refresh();
		x = getch();
		movexy(0, pline);
		addstr("  ");
		switch (x) {
			case 'q':
			case 27:
				return 0;
			case 13:
			case 10:
				ModifyPartei(findfaction(atoi(pointer->s)));
				break;
			case 'D':
				RemovePartei();
				return 1;
			case KEY_DOWN:
				if (pointer->next) {
					pline++;
					pointer = pointer->next;
					if (pline == SY - 1) {
						x = 20;
						do {
							oben = oben->next;
							unten = unten->next;
							x--;
							pline--;
						} while (x && unten /*->next*/ );
						x = -1;
					}
				} else
					beep();
				break;
			case KEY_UP:
				if (pointer->prev) {
					pline--;
					pointer = pointer->prev;
					if (pline < 0) {
						x = 20;
						do {
							oben = oben->prev;
							unten = unten->prev;
							x--;
							pline++;
						} while (x && oben->prev);
						x = -1;
					}
				} else
					beep();
				break;
			case KEY_NPAGE:
			case KEY_RIGHT:
				for (x = 20; x && unten; x--) {
					oben = oben->next;
					pointer = pointer->next;
					unten = unten->next;
				}
				x = -1;
				break;
			case KEY_PPAGE:
			case KEY_LEFT:
				for (x = 20; x && oben->prev; x--) {
					oben = oben->prev;
					pointer = pointer->prev;
					unten = unten->prev;
				}
				x = -1;
				break;
			case '/':
				strcpy(buf, my_input(0, 0, 0, "Partei Nr. oder Name: ", NULL));
				touchwin(stdscr);	/* redraw erzwingen */
				for (tmp = P; tmp; tmp = tmp->next) {
					s = tmp->s;
					while (s && strncasecmp(buf, s, strlen(buf))) {
						s = strchr(s, ' ');
						if (s)
							s++;
					}
					if (s)
						break;
				}
				if (tmp) {
					pointer = tmp;
					pline = 0;
					if (tmp->prev) {
						tmp = tmp->prev;
						pline++;
					}
					oben = tmp;
					x = -1;
				} else
					beep();
				break;
		}
	}
}
