/* vi: set ts=2:
 *
 *	$Id: map_region.c,v 1.3 2001/02/03 13:45:34 enno Exp $
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
#include <faction.h>
#include <plane.h>
#include <race.h>
#include <region.h>
#include <ship.h>
#include <unit.h>

/* util includes */
#include <base36.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

void
ncat(int n)
{
	static char s[10];
	sprintf(s, " %d", n);
	sncat(buf, s, BUFSIZE);
}

int rbottom;
dbllist *reglist = NULL;
static dbllist *runten = NULL, *roben = NULL;
#define LASTLINE SY-2
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

	if (r->terrain != T_OCEAN && r->terrain!=T_FIREWALL) {
		sprintf(buf, " %hd Runden alt:", r->age);
		adddbllist(&reglist, buf);
		sprintf(buf, " %d Bauern, %d(%d) Silber", rpeasants(r), rmoney(r), count_all_money(r));
		adddbllist(&reglist, buf);
		sprintf(buf, " %d Pferde, %d ", rhorses(r), rtrees(r));
		if (fval(r,RF_MALLORN))
			sncat(buf, "Mallorn", BUFSIZE);
		else
			sncat(buf, "Bäume", BUFSIZE);
		adddbllist(&reglist, buf);

		if (r->terrain == T_MOUNTAIN || r->terrain == T_GLACIER) {
			sprintf(buf, " %d Eisen, %d Laen", riron(r), rlaen(r));
			adddbllist(&reglist, buf);
		}
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
		strcpy(buf, "Burgen:");
		if (!r->buildings) {
			sncat(buf, " keine", BUFSIZE);
			adddbllist(&reglist, buf);
		} else if (full) {
			adddbllist(&reglist, buf);
			for (b = r->buildings; b; b = b->next) {
				adddbllist(&reglist, Buildingid(b));
				for (u = r->units; u; u = u->next)
					if (u->building == b && fval(u, FL_OWNER)) {
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
			ncat(d);
			adddbllist(&reglist, buf);
		}
		NL(reglist);
	}
	strcpy(buf, "Schiffe:");
	if (!r->ships) {
		sncat(buf, " keine", BUFSIZE);
		adddbllist(&reglist, buf);
	} else if (full) {
		adddbllist(&reglist, buf);
		for (sh = r->ships; sh; sh = sh->next) {
			adddbllist(&reglist, Shipid(sh));
			for (u = r->units; u; u = u->next)
				if (u->ship == sh && fval(u, FL_OWNER)) {
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
		ncat(d);
		adddbllist(&reglist, buf);
	}
	NL(reglist);

	if (!factions)
		return;

	strcpy(buf, "Parteien:");

	if (!r->units) {
		sncat(buf, " keine", BUFSIZE);
		adddbllist(&reglist, buf);
	} else {
		adddbllist(&reglist, buf);

		for (f = factions; f; f = f->next)
			f->num_people = f->nunits = 0;
		for (u = r->units; u; u = u->next) {
			if (u->faction) {
				u->faction->nunits++;
				u->faction->num_people += u->number;
			} else
				fprintf(stderr,"Unit %s hat keine faction!\n",unitid(u));
		}
		for (f = factions; f; f = f->next)
			if (f->nunits) {
				sprintf(buf, " %-29.29s (%s)", f->name, factionid(f));
				adddbllist(&reglist, buf);
				sprintf(buf, "  Einheiten: %d; Leute: %d %c",
				   f->nunits, f->num_people, Tchar[f->race]);
				adddbllist(&reglist, buf);
			}

		for (d = RC_UNDEAD; d < MAXRACES; d++)
			ecount[d] = count[d] = 0;
		for (u = r->units; u; u = u->next)  {
			if (u->race >= RC_UNDEAD)
				pp=1;
			ecount[u->race]++;
			count[u->race] += u->number;
		}
		if (pp) {
			NL(reglist);
			adddbllist(&reglist, "Monster, Zauber usw.:");
			for (d = RC_UNDEAD; d < MAXRACES; d++) {
				if (count[d]) {
					sprintf(buf, "  %s: %d in %d", race[d].name[0], count[d], ecount[d]);
					adddbllist(&reglist, buf);
				}
			}
		}
	}
	NL(reglist);
}
