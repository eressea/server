/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "zombies.h"

/* kernel includes */
#include <unit.h>
#include <faction.h>
#include <region.h>

/* libc includes */
#include <stdlib.h>

#define UNDEAD_MIN                  90	/* mind. zahl vor weg gehen */
#define UNDEAD_BREAKUP              25	/* chance dafuer */
#define UNDEAD_BREAKUP_FRACTION     (25+rand()%70)	/* anteil der weg geht */

#define age_chance(a,b,p) (max(0,a-b)*p)

void
age_undead(unit *u)
{
	region *r = u->region;
	int n = 0;

#if UNDEAD_REPRODUCTION > 0
	if(landregion(rterrain(r))) {
		int m;
#ifndef SLOW_UNDEADS
		if (u->number>100) {
			int k = u->number;
			for (m=0;m!=100;++m) {
				int d = k/(100-m);
				k-=d;
				if (rand() % 100 < UNDEAD_REPRODUCTION)
					n+=d;
			}
			assert(k==0);
		} else
#endif
		for (m = u->number; m; m--)
			if (rand() % 100 < UNDEAD_REPRODUCTION)
				n++;
		set_number(u, u->number + n);
		u->hp += n * unit_max_hp(u);
		n = rpeasants(r) - n;
		n = max(0, n);
		rsetpeasants(r, n);
	}
#endif
	/* untote, die einer partei angehoeren, koennen sich
	 * absplitten, anstatt sich zu vermehren. monster
	 * untote vermehren sich nur noch */

	if (u->number > UNDEAD_MIN && u->faction->no != MONSTER_FACTION && rand() % 100 < UNDEAD_BREAKUP) {
		int m;
		unit *u2;

		n = 0;
		for (m = u->number; m; m--)
			if (rand() % 100 < UNDEAD_BREAKUP_FRACTION)
				n++;
		u2 = make_undead_unit(r, findfaction(MONSTER_FACTION), 0, new_race[RC_UNDEAD]);
		transfermen(u, u2, u->number - n);
		u2->building = u->building;
		u2->ship = u->ship;
	}
}

void
age_skeleton(unit *u)
{
	if (u->faction->no == 0 && rand()%100 < age_chance(u->age, 27, 1)) {
		int n = max(1,u->number/2);
		double q = (double) u->hp / (double) (unit_max_hp(u) * u->number);
		u->race = new_race[RC_SKELETON_LORD];
		u->irace = new_race[RC_SKELETON_LORD];
		scale_number(u,n);
		u->hp = (int) (unit_max_hp(u) * u->number * q);
	}
}

void
age_zombie(unit *u)
{
	if (u->faction->no == 0 && rand()%100 < age_chance(u->age, 27, 1)) {
		int n = max(1,u->number/2);
		double q = (double) u->hp / (double) (unit_max_hp(u) * u->number);
		u->race = new_race[RC_ZOMBIE_LORD];
		u->irace = new_race[RC_ZOMBIE_LORD];
		scale_number(u,n);
		u->hp = (int) (unit_max_hp(u) * u->number * q);
	}
}

void
age_ghoul(unit *u)
{
	if (u->faction->no == 0 && rand()%100 < age_chance(u->age, 27, 1)) {
		int n = max(1,u->number/2);
		double q = (double) u->hp / (double) (unit_max_hp(u) * u->number);
		u->race = new_race[RC_GHOUL_LORD];
		u->irace = new_race[RC_GHOUL_LORD];
		scale_number(u,n);
		u->hp = (int) (unit_max_hp(u) * u->number * q);
	}
}

