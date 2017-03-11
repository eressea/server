/*
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2015
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <platform.h>

/* kernel includes */
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <util/rng.h>

#define age_chance(a,b,p) (MAX(0,a-b)*p)

#define DRAGONAGE                   27
#define WYRMAGE                     68

void age_firedragon(unit * u)
{
    if (u->number > 0 && rng_int() % 100 < age_chance(u->age, DRAGONAGE, 1)) {
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        u_setrace(u, get_race(RC_DRAGON));
        u->irace = NULL;
        scale_number(u, 1);
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}

void age_dragon(unit * u)
{
    if (u->number > 0 && rng_int() % 100 < age_chance(u->age, WYRMAGE, 1)) {
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        u_setrace(u, get_race(RC_WYRM));
        u->irace = NULL;
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}
