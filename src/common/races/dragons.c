/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "dragons.h"

/* kernel includes */
#include <region.h>
#include <unit.h>

/* util includes */
#include <util/rng.h>

#define age_chance(a,b,p) (max(0,a-b)*p)

#define DRAGONAGE                   27
#define WYRMAGE                     68

void
age_firedragon(unit *u)
{
	if (u->number>0 && rng_int()%100 < age_chance(u->age, DRAGONAGE, 1)) {
		double q = (double) u->hp / (double) (unit_max_hp(u) * u->number);
		u->race = new_race[RC_DRAGON];
		u->irace = new_race[RC_DRAGON];
		scale_number(u,1);
		u->hp = (int) (unit_max_hp(u) * u->number * q);
	}
}

void
age_dragon(unit *u)
{
	if (u->number>0 && rng_int()%100 < age_chance(u->age, WYRMAGE, 1)) {
		double q = (double) u->hp / (double) (unit_max_hp(u) * u->number);
		u->race = new_race[RC_WYRM];
		u->irace = new_race[RC_WYRM];
		u->hp = (int) (unit_max_hp(u) * u->number * q);
	}
}

