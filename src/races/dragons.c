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

static int age_chance(int a, int b, int p) {
    int r = (a - b) * p;
    return (r < 0) ? 0 : r;
}

#define DRAGONAGE 27
#define WYRMAGE 68

static void evolve_dragon(unit * u, const struct race *rc) {
    scale_number(u, 1);
    u_setrace(u, rc);
    u->irace = NULL;
    u->hp = unit_max_hp(u);
}

void age_firedragon(unit * u)
{
    if (u->number > 0 && rng_int() % 100 < age_chance(u->age, DRAGONAGE, 1)) {
        evolve_dragon(u, get_race(RC_DRAGON));
    }
}

void age_dragon(unit * u)
{
    if (u->number > 0 && rng_int() % 100 < age_chance(u->age, WYRMAGE, 1)) {
        evolve_dragon(u, get_race(RC_WYRM));
    }
}
