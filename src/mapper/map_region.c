/* vi: set ts=2:
 *
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
#include <ctype.h>
#include <eressea.h>
#include "mapper.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

/* modules includes */

/* util includes */
#include <base36.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

void
incat(char *buf, const int n, const size_t bufsize)
{
	char s[10];
	snprintf(s, 9, "%d", n);
	sncat(buf, s, bufsize);
}

int rbottom;
dbllist *reglist = NULL;
static dbllist *runten = NULL, *roben = NULL;
#define LASTLINE SY-3
/* Letzte Zeile, in der Regionsinfo bei der Karte kommt */


static void
ClearRegion(void)
{
	int line;
	refresh();
	for (line = 0; line < LASTLINE; line++) {
		movexy(SX - 38, line);
		clrtoeol();
	}
}

void
ScrollRegList(int dir)
{
	dbllist *hlp;
	int line;
	if ((dir == -1 && !runten->next) || (dir == 1 && !roben->prev))
		return;

	switch (dir) {
	case -1:
		for (line = 0; line < SY - 4 && runten->next; line++) {
			roben = roben->next;
			runten = runten->next;
		}
		ClearRegion();
		for (line = 0, hlp = roben; line < LASTLINE && hlp; line++, hlp = hlp->next) {
			movexy(SX - 37, line);
			addstr(hlp->s);
		}
		rbottom = line;
		break;
	case 1:
		for (line = 0; line < SY - 4 && roben->prev; line++) {
			roben = roben->prev;
			runten = runten->prev;
		}
		ClearRegion();
		for (line = 0, hlp = roben; line < LASTLINE && hlp; line++, hlp = hlp->next) {
			movexy(SX - 37, line);
			addstr(hlp->s);
		}
		rbottom = line;
		break;
	}
}

void
saddstr(char *s)
{
	if (s[0] < ' ')
		s++;
	addstr(s);
}

void
DisplayRegList(int neu)
{
	int line;
	dbllist *hlp;
	ClearRegion();

	if (!reglist) {
		movexy(SX - 30, 0);
		addstr("undefiniert");
		refresh();
		return;
	}
	if (neu)
		runten = reglist;
	else
		runten = roben;

	for (line = 0, hlp = runten; line < LASTLINE && hlp; line++, hlp = hlp->next) {
		movexy(SX - 37, line);
		saddstr(hlp->s);
		if (runten->next)
			runten = runten->next;
	}
	rbottom = line;
	if (neu)
		roben = reglist;
	refresh();
}

void
SpecialFunction(region *r)
{
	WINDOW *win;
  variant zero_effect;
  zero_effect.i = 0;

	win = openwin(60, 5, "< Specials Regions >");
	wmove(win, 1, 2);
	wAddstr("1 - set Godcurse (n.imm), 2 - set Peace-Curse (imm)");
	wmove(win, 2, 2);
	wrefresh(win);
	switch(getch()) {
	case '1':
		if (get_curse(r->attribs, ct_find("godcursezone"))==NULL) {
			curse * c = create_curse(NULL, &r->attribs, ct_find("godcursezone"),
				100, 100, zero_effect, 0);
			curse_setflag(c, CURSE_ISNEW|CURSE_IMMUNE);
			modified = 1;
			break;
		}
	case '2':
		if(!is_cursed_internal(r->attribs, ct_find("peacezone"))) {
			curse * c = create_curse(NULL, &r->attribs, ct_find("peacezone"), 100, 2, zero_effect, 0);
			curse_setflag(c, CURSE_IMMUNE);
			modified = 1;
		}
	default:
		break;
	}
	delwin(win);
}

static int
count_all_money(const region * r)
{
  const unit *u;
  int m = rmoney(r);

  for (u = r->units; u; u = u->next)
    m += get_money(u);

  return m;
}

char Tchar[MAXRACES] = "ZEOGMTDIHK~uifdwtbrsz";

void
showregion(region * r, char full)
{
	unit *u;
	building *b;
	ship *sh;
	faction *f;
	int d,pp=0, ecount[MAXRACES], count[MAXRACES];
	char str[256];
#if NEW_RESOURCEGROWTH
	int iron = -1, ironlevel = -1,
	    laen = -1, laenlevel = -1,
			stone = -1, stonelevel = -1;
	struct rawmaterial *res;
#endif

	if (reglist) {
		freelist(reglist);
		reglist = NULL;
	}
	if (!r) return;

	memset(ecount, 0, sizeof(ecount));
	strncpy(str, rname(r, NULL), 24);
	str[24] = 0;
	sprintf(buf, "%s (%d,%d)", str, r->x, r->y);
	adddbllist(&reglist, buf);
	memset(str,'¯',strlen(buf)+1);
	str[strlen(buf)+1] = 0;
	adddbllist(&reglist, str);

	sprintf(buf, " %hd turns old:", r->age);
	adddbllist(&reglist, buf);
	if (r->terrain != T_OCEAN && r->terrain!=T_FIREWALL) {
		sprintf(buf, " %d peasants, %d(%d) silver", rpeasants(r), rmoney(r), count_all_money(r));
		adddbllist(&reglist, buf);
		sprintf(buf, " %d horses, %d/%d/%d ",
				rhorses(r), rtrees(r,2), rtrees(r,1), rtrees(r,0));
		if (fval(r,RF_MALLORN))
			sncat(buf, "mallorn", BUFSIZE);
		else
			sncat(buf, "trees", BUFSIZE);
		adddbllist(&reglist, buf);

#if NEW_RESOURCEGROWTH
		for(res=r->resources;res;res=res->next) {
			const item_type * itype = resource2item(res->type->rtype);
			if(itype == olditemtype[I_IRON]) {
				iron = res->amount;
				ironlevel = res->level + itype->construction->minskill - 1;
			} else if(itype == olditemtype[I_LAEN]) {
				laen = res->amount;
				laenlevel = res->level + itype->construction->minskill - 1;
			} else if(itype == olditemtype[I_STONE]) {
				stone = res->amount;
				stonelevel = res->level + itype->construction->minskill - 1;
			}
		}
		strcpy(buf, " ");
		if(iron != -1) {
			incat(buf, iron, BUFSIZE);
			sncat(buf, " iron/", BUFSIZE);
			incat(buf, ironlevel, BUFSIZE);
		}
		if(laen != -1) {
			if(iron != -1) {
				sncat(buf, ", ", BUFSIZE);
			}
			incat(buf, laen, BUFSIZE);
			sncat(buf, " laen/", BUFSIZE);
			incat(buf, laenlevel, BUFSIZE);
		}
		if(iron != -1 || laen != -1) {
			adddbllist(&reglist, buf);
		}
		if(stone != -1) {
			snprintf(buf, BUFSIZE, " %d stone/%d", stone, stonelevel);
			adddbllist(&reglist, buf);
		}
#else
		if (riron(r) > 0 || rlaen(r) > 0) {
			sprintf(buf, " %d Eisen, %d Laen", riron(r), rlaen(r));
			adddbllist(&reglist, buf);
		}
#endif
	}
	if (fval(r, RF_CHAOTIC)) {
		adddbllist(&reglist, "chaotisch");
	}
	if (r->planep) {
		strcpy(buf,"Plane: ");
		strncpy(str, r->planep->name, 30);
		str[30] = 0; sncat(buf, str, BUFSIZE);
		sncat(buf, " (", BUFSIZE);
		sncat(buf, r->planep->name, BUFSIZE);
		sncat(buf, ")", BUFSIZE);
		adddbllist(&reglist, buf);
	}
	NL(reglist);

	if (r->terrain != T_OCEAN) {
		strcpy(buf, "buildings:");
		if (!r->buildings) {
			sncat(buf, " keine", BUFSIZE);
			adddbllist(&reglist, buf);
		} else if (full) {
			adddbllist(&reglist, buf);
			for (b = r->buildings; b; b = b->next) {
				adddbllist(&reglist, Buildingid(b));
				for (u = r->units; u; u = u->next)
					if (u->building == b && fval(u, UFL_OWNER)) {
						strncpy(str, u->name, 27);
						str[27] = 0;
						sprintf(buf, "  %s (%s)", str, unitid(u));
						adddbllist(&reglist, buf);
						strncpy(str, factionname(u->faction), 34);
						str[34] = 0;
						sprintf(buf, "   %s", str);
						adddbllist(&reglist, buf);
						break;
					}
				if (!u)
					adddbllist(&reglist, "  steht leer");
			}
		} else {
			d = 0;
			for (b = r->buildings; b; b = b->next)
				d++;
			sncat(buf, " ", BUFSIZE);
			incat(buf, d, BUFSIZE);
			adddbllist(&reglist, buf);
		}
		NL(reglist);
	}
	strcpy(buf, "ships:");
	if (!r->ships) {
		sncat(buf, " keine", BUFSIZE);
		adddbllist(&reglist, buf);
	} else if (full) {
		adddbllist(&reglist, buf);
		for (sh = r->ships; sh; sh = sh->next) {
			adddbllist(&reglist, Shipid(sh));
			for (u = r->units; u; u = u->next)
				if (u->ship == sh && fval(u, UFL_OWNER)) {
					strncpy(str, u->name, 28);
					str[28] = 0;
					sprintf(buf, "  %s (%s)", str, unitid(u));
					adddbllist(&reglist, buf);
					sprintf(buf, "  %s", factionname(u->faction));
					adddbllist(&reglist, buf);
					break;
				}
			if (!u)
				adddbllist(&reglist, "  ohne Besitzer");
		}
	} else {
		d = 0;
		for (sh = r->ships; sh; sh = sh->next)
			d++;
		sncat(buf, " ", BUFSIZE);
		incat(buf, d, BUFSIZE);
		adddbllist(&reglist, buf);
	}
	NL(reglist);

	if (!factions)
		return;

	strcpy(buf, "factions:");

	if (!r->units) {
		sncat(buf, " keine", BUFSIZE);
		adddbllist(&reglist, buf);
	} else {
		adddbllist(&reglist, buf);

		for (f = factions; f; f = f->next)
			f->num_people = f->no_units = 0;
		for (u = r->units; u; u = u->next) {
			if (u->faction) {
				u->faction->no_units++;
				u->faction->num_people += u->number;
			} else
				fprintf(stderr,"Unit %s hat keine faction!\n",unitid(u));
		}
		for (f = factions; f; f = f->next)
			if (f->no_units) {
        if (alliances==NULL) {
          sprintf(buf, " %-29.29s (%s)", f->name, factionid(f));
        } else if (f->alliance != NULL) {
          sprintf(buf, " %-26.26s (%s/%d)", f->name, factionid(f), f->alliance->id);
        } else {
          sprintf(buf, " %-26.26s (%s/-)", f->name, factionid(f));
        }
				adddbllist(&reglist, buf);
				sprintf(buf, "  Einheiten: %d; Leute: %d %c",
				   f->no_units, f->num_people, Tchar[old_race(f->race)]);
				adddbllist(&reglist, buf);
			}

		for (d = RC_UNDEAD; d < MAXRACES; d++)
			ecount[d] = count[d] = 0;
		for (u = r->units; u; u = u->next)  {
			if (u->race >= new_race[RC_UNDEAD])
				pp=1;
			ecount[old_race(u->race)]++;
			count[old_race(u->race)] += u->number;
		}
		if (pp) {
			NL(reglist);
			adddbllist(&reglist, "Monster, Zauber usw.:");
			for (d = RC_UNDEAD; d < MAXRACES; d++) {
				if (count[d]) {
					sprintf(buf, "  %s: %d in %d", new_race[d]->_name[0], count[d], ecount[d]);
					adddbllist(&reglist, buf);
				}
			}
		}
	}
	NL(reglist);
}
