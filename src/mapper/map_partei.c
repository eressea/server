/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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

#include "autoseed.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/save.h>
#include <kernel/skill.h>
#include <kernel/study.h>
#include <kernel/unit.h>

/* util includes */
#include <util/base36.h>
#include <util/goodies.h>
#include <language.h>

/* libc includes */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef __USE_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

const char * orderfile = NULL;

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

static int
days2level(int days)
{
	int l = 0;
	while (level_days(l)<=days) ++l;
	return l-1;
}

static void
change_level(unit * u, skill_t sk, int bylevel)
{
	skill * sv = get_skill(u, sk);
	assert(bylevel>0);
	if (sv==0) sv = add_skill(u, sk);
	sk_set(sv, sv->level+bylevel);
}

static void
give_latestart_bonus(region *r, unit *u, int b)
{
	int bsk = days2level(b*30);
	change_level(u, SK_OBSERVATION, bsk);
	change_money(u, 200*b);

	{
		unit *u2 = createunit(r, u->faction, 1, u->race);
		change_level(u2, SK_TACTICS, bsk);
		u2->irace = u->irace;
/*		fset(u2, UFL_PARTEITARNUNG); */
	}

	{
		unit *u2 = createunit(r, u->faction, 2*b, u->race);
		change_level(u2, SK_SPEAR, 3);
		change_level(u2, SK_TAXING, 3);
		change_item(u2, I_SPEAR, u2->number);
		u2->irace = u->irace;
/*		fset(u2, UFL_PARTEITARNUNG); */
	}
}

dropout * dropouts = NULL;

void
read_dropouts(const char * filename)
{
	FILE * F = fopen(filename, "r");
	if (F==NULL) return;
	for (;;) {
		char email[64], race[20];
		int age, x, y;
		if (fscanf(F, "%s %s %d %d %d", email, race, &age, &x, &y)<=0) break;
		if (age<=2) {
			region * r = findregion(x, y);
			if (r) {
				dropout * drop = calloc(sizeof(dropout), 1);
				drop->race = rc_find(race);
				if (drop->race==NULL) drop->race = findrace(race, default_locale);
				drop->x = x;
				drop->y = y;
				drop->fno = -1;
				drop->next = dropouts;
				dropouts = drop;
			}
		}
	}
	fclose(F);
}

void
seed_dropouts(void)
{
	dropout ** dropp = &dropouts;
	while (*dropp) {
		dropout *drop = *dropp;
		region * r = findregion(drop->x, drop->y);
		if (r) {
			boolean found = false;
			newfaction **nfp = &newfactions;
			unit * u;
			for (u=r->units;u;u=u->next) if (u->faction->no!=drop->fno) break;
			if (u==NULL) while (*nfp) {
				newfaction * nf = *nfp;
				if (nf->race==drop->race && !nf->bonus) {
					unit * u = addplayer(r, addfaction(nf->email, nf->password, nf->race, nf->lang,
						nf->subscription));
					u->faction->alliance = nf->allies;
					++numnewbies;
					if (nf->bonus) give_latestart_bonus(r, u, nf->bonus);
					found=true;
					*dropp = drop->next;
					*nfp = nf->next;
					free(nf);
					break;
				}
				nfp = &nf->next;
			}
			if (!found) dropp=&drop->next;
		} else {
			*dropp = drop->next;
		}
	}
}

void
read_newfactions(const char * filename)
{
  FILE * F = fopen(filename, "r");
  if (F==NULL) return;
  for (;;) {
		faction * f = factions;
		char race[20], email[64], lang[8], password[16];
		newfaction *nf, **nfi;
		int bonus, subscription;
		int alliance = 0;
		
		if (alliances!=NULL) {
			/* email;race;locale;startbonus;subscription;alliance */
			if (fscanf(F, "%s %s %s %d %d %s %d", email, race, lang, &bonus, &subscription, password, &alliance)<=0) break;
		} else {
			/* email;race;locale;startbonus;subscription */
			if (fscanf(F, "%s %s %s %d %d %s", email, race, lang, &bonus, &subscription, password)<=0) break;
		}
		while (f) {
			if (strcmp(f->email, email)==0 && f->subscription) {
				break;
			}
			f = f->next;
		}
		if (f) continue; /* skip the ones we've already got */
		for (nf=newfactions;nf;nf=nf->next) {
			if (strcmp(nf->email, email)==0) break;
		}
		if (nf) continue;
		nf = calloc(sizeof(newfaction), 1);
		if (set_email(&nf->email, email)!=0) {
			log_error(("Invalid email address for subscription %s: %s\n", itoa36(subscription), email));
		}
		nf->password = strdup(password);
		nf->race = rc_find(race);
		nf->subscription = subscription;
		if (alliances!=NULL) {
			struct alliance * al = findalliance(alliance);
			if (al==NULL) {
				char zText[64];
				sprintf(zText, "Allianz %d", alliance);
				al = makealliance(alliance, zText);
			}
			nf->allies = al;
		} else {
			nf->allies = NULL;
		}
		if (nf->race==NULL) nf->race = findrace(race, default_locale);
		nf->lang = find_locale(lang);
		nf->bonus = bonus;
		assert(nf->race && nf->email && nf->lang);
		nfi = &newfactions;
		while (*nfi) {
			if ((*nfi)->race==nf->race) break;
			nfi=&(*nfi)->next;
		}
		nf->next = *nfi;
		*nfi = nf;
  }
  fclose(F);
}

newfaction *
select_newfaction(const struct race * rc)
{
	selection *prev, *ilist = NULL, **iinsert;
	selection *selected = NULL;
	newfaction *player = newfactions;

	if (!player) return NULL;
	insert_selection(&ilist, NULL, strdup("new player"), NULL);
	iinsert=&ilist->next;
	prev = ilist;

	while (player) {
		if (rc==NULL || player->race==rc) {
			char str[80];
      if (alliances!=NULL) {
        snprintf(str, 70, "%s %d %s %s", player->bonus?"!":" ", player->allies?player->allies->id:0, locale_string(default_locale, rc_name(player->race, 0)), player->email);
      } else {
        snprintf(str, 70, "%s %s %s", player->bonus?"!":" ", locale_string(default_locale, rc_name(player->race, 0)), player->email);
      }
			insert_selection(iinsert, prev, strdup(str), (void*)player);
			prev = *iinsert;
			iinsert = &prev->next;
		}
		player=player->next;
	}
	selected = do_selection(ilist, "Partei", NULL, NULL);
	if (selected==NULL) return NULL;
	return (newfaction*)selected->data;
}

void
NeuePartei(region * r)
{
	int i, q, y;
	char email[INPUT_BUFSIZE+1];
	newfaction * nf, **nfp;
	const struct locale * lang;
	const struct race * frace;
	int late, subscription = 0;
	unit *u;
	const char * passwd = NULL;
	int locale_nr;
	faction * f;

	if (!r->land) {
		warnung(0, "Ungeeignete Region! <Taste>");
		return;
	}

	nf = select_newfaction(NULL);

	if (nf!=NULL) {
		frace = nf->race;
		subscription = nf->subscription;
		late = nf->bonus;
		lang = nf->lang;
		passwd = nf->password;
		strcpy(email, nf->email);
		if (late) {
			WINDOW *win = openwin(SX - 10, 3, "< Neue Partei einfügen >");
			if(r->age >= 5)
				late = (int) map_input(win, 2, 1, "Startbonus", -1, 99, r->age/2);
			else
				late = (int) map_input(win, 2, 1, "Startbonus", -1, 99, 0);
			delwin(win);
		}
	} else {
		WINDOW *win = openwin(SX - 10, 12, "< Neue Partei einfügen >");

		strcpy(buf, my_input(win, 2, 1, "EMail-Adresse (Leer->Ende): ", NULL));
		if (!buf[0]) {
			delwin(win);
			return;
		}

		strcpy(email, buf);
		for (f=factions;f;f=f->next) {
			if (strcmp(email, f->email)==0 && f->age==0) {
				warnung(0, "Neue Partei mit dieser Adresse existiert bereits.");
				delwin(win);
				return;
			}
		}

		y = 3;
		q = 0;
		wmove(win, y, 4);
		for (i = 0; i < MAXRACES; i++) if(playerrace(new_race[i])) {
		  sprintf(buf, "%d=%s; ", i, new_race[i]->_name[0]);
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
			int nrace = (char) map_input(win, 2, y, "Rasse", -1, MAXRACES-1, 0);
			if (nrace == -1) {
				delwin(win);
				return;
			}
			frace = new_race[nrace];
		} while(!playerrace(frace));

		y++;
		late = (int) map_input(win, 2, y, "Startbonus", -1, 99, 0);
		if(late == -1) {
			delwin(win);
			return;
		}
		i = 0; q = 0; y++;
		wmove(win, y, 4);
		while(localenames[i] != NULL) {
			sprintf(buf, "%d=%s; ", i, localenames[i]);
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
		lang = find_locale(localenames[locale_nr]);

		delwin(win);
	}

	/* remove duplicate email addresses */
	nfp=&newfactions;
	while (*nfp) {
		newfaction * nf = *nfp;
		if (strcmp(email, nf->email)==0) {
			*nfp = nf->next;
			free(nf);
		}
		else nfp = &nf->next;
	}
	modified = 1;
	u = addplayer(r, addfaction(email, passwd, frace, lang, subscription));
	++numnewbies;

	if(late) give_latestart_bonus(r, u, late);

	sprintf(buf, "newuser %s", email);
	system(buf);
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
	wprintw(win, (NCURSES_CONST char*)"Rasse: %s", f->race->_name[1]);
	wmove(win, 2, 2);
	wprintw(win, (NCURSES_CONST char*)"Einheiten: %d", f->no_units);
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
				if (u->faction == f && has_skill(u, SK_MAGIC)) {
					if (fval(f, FL_DH))
						waddnstr(win, ", ", -1);
					wprintw(win, (NCURSES_CONST char*)"%s(%d): %d", unitid(u), u->number, get_level(u, SK_MAGIC));
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
				if (u->faction == f && has_skill(u, SK_ALCHEMY)) {
					if (fval(f, FL_DH))
						waddnstr(win, ", ", -1);
					wprintw(win, (NCURSES_CONST char*)"%s(%d): %d", unitid(u), u->number, get_level(u, SK_ALCHEMY));
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
			/*wborder(mywin, '|', '|', '-', '-', '.', '.', '`', '\'');*/
			wborder(mywin, 0, 0, 0, 0, 0, 0, 0, 0);
			Movexy(3, 0);
			Addstr("< Rasse wählen >");
			wrefresh(mywin);
			y = 2;
			q = 0;
			wmove(mywin, y, 4);
			for (i = 1; i < MAXRACES; i++) {
				sprintf(buf, "%d=%s; ", i, new_race[i]->_name[0]);
				q += strlen(buf);
				if (q > SX - 25) {
					q = strlen(buf);
					y++;
					wmove(mywin, y, 4);
				}
				wAddstr(buf);
			}
			q = map_input(mywin, 2, 1, "Rasse", 1, MAXRACES-1, old_race(f->race));
			delwin(mywin);
			touchwin(stdscr);
			touchwin(win);
			if (new_race[q] != f->race) {
				modified = 1;
				f->race = new_race[q];
				wmove(win, 1, 2);
				wprintw(win, (NCURSES_CONST char*)"Rasse: %s", f->race->_name[1]);
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

	for (f = factions; f; f = f->next) {
	  if (SX > 104)
		sprintf(buf, "%4s: %-30.30s %-12.12s %-24.24s", factionid(f),
				f->name, f->race->_name[1], f->email);
	  else
		sprintf(buf, "%4s: %-30.30s %-12.12s", factionid(f),
				f->name, f->race->_name[1]);
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
				{
					char fno[5];
					strncpy(fno, pointer->s, 4);
					fno[4]=0;
					ModifyPartei(findfaction(atoi36(fno)));
				}
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
