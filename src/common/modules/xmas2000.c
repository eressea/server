/* vi: set ts=2:
 *
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

#include <config.h>
#include <eressea.h>
#include "xmas2000.h"

/* modules includes */
#include "xmas.h"

/* kernel includes */
#include <plane.h>
#include <item.h>
#include <unit.h>
#include <region.h>
#include <building.h>
#include <movement.h>
#include <event.h>
#include <faction.h>
#include <race.h>

/* util includes */
#include <goodies.h>
#include <resolve.h>
#include <base36.h>

#include <stdlib.h>

static void
make_gates(region * r)
{
	const building_type * btype = bt_find("xmas_exit");
	building * b;
	if (btype==NULL)
		btype = bt_make("xmas_exit", BTF_NOBUILD | BTF_INDESTRUCTIBLE, -1, 2, 10);
	else {
		b = r->buildings;
		while (b!=NULL && b->type!=btype) b = b->next;
		if (b!=NULL) return; /* gibt schon einen */
	}

	b = new_building(btype, r, NULL);
	b->size = btype->maxsize;
	b->name = strdup("Der Weg nach Hause");
	b->display = strdup("Achtung, hier gibt es die Geschenke!");
	add_trigger(&b->attribs, "timer", trigger_xmasgate(b));
}

void
create_xmas2000(int x, int y)
{
	const int prefixes = 18;
	const char *prefix[] = {
		"Artige", "Brave", "Liebe", "Nette", "Anständige",
	    "Rechtschaffene", "Tugendhafte", "Sittsame", "Ehrbare", "Keusche",
		"Tugendsame", "Züchtige", "Fromme", "Musterhafte", "Manierliche",
		"Tadellose", "Unverdorbene", "Höfliche"
	};
	plane * xmas = getplanebyname("Nordpol");
	faction * f;
	region * r = findregion(x, y);
	direction_t dir;

	if (r!=NULL) {
		make_santa(r);
		return;
	}
	if (xmas==NULL) xmas = create_new_plane(2000, "Nordpol", x-1, x+1, y-1, y+1, PFL_NORECRUITS|PFL_NOALLIANCES|PFL_LOWSTEALING|PFL_NOGIVE|PFL_NOATTACK|PFL_NOMAGIC|PFL_NOSTEALTH|PFL_NOTEACH|PFL_NOBUILD|PFL_NOFEED|PFL_FRIENDLY);
	r = new_region(x, y);
	terraform(r, T_PLAIN);
	set_string(&r->land->name, "Weihnachtsinsel");
#if GROWING_TREES
	rsettrees(r, 2, 1000);
#else
	rsettrees(r, 1000);
#endif
	rsetpeasants(r, 0);
	rsetmoney(r, 0);
	for (dir=0;dir!=MAXDIRECTIONS;++dir) {
		region * n = findregion(x + delta_x[dir], y + delta_y[dir]);
		if (n==NULL) n = new_region(x + delta_x[dir], y + delta_y[dir]);
		terraform(n, T_OCEAN);
	}
	santa_comes_to_town(r, make_santa(r), NULL);
	for (f = factions; f != NULL; f = f->next) if (f->alive && f->units) {
		char zText[128];
		unit * u;

		u = createunit(r, f, 2, f->race);
		if (f->race==new_race[RC_DAEMON]) u->irace = new_race[RC_HUMAN];
		sprintf(zText, "%s %s", prefix[rand()%prefixes], LOC(u->faction->locale, rc_name(u->irace, 1)));
		fset(u, FL_PARTEITARNUNG);
		set_string(&u->name, zText);
	}
	make_gates(r);
}
