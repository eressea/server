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
#include <kernel/config.h>

/* kernel includes */
#include <kernel/race.h>
#include <kernel/order.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/region.h>

/* util iclude */
#include <util/rng.h>

#include "monster.h"

/* libc includes */
#include <stdlib.h>

#define UNDEAD_MIN                  90  /* mind. zahl vor weg gehen */
#define UNDEAD_BREAKUP              25  /* chance dafuer */
#define UNDEAD_BREAKUP_FRACTION     (25+rng_int()%70)   /* anteil der weg geht */

#define age_chance(a,b,p) (_max(0,a-b)*p)

void make_undead_unit(unit * u)
{
    free_orders(&u->orders);
    name_unit(u);
    fset(u, UFL_ISNEW);
}

void age_undead(unit * u)
{
    region *r = u->region;
    int n = 0;

    /* untote, die einer partei angehoeren, koennen sich
     * absplitten, anstatt sich zu vermehren. monster
     * untote vermehren sich nur noch */

    if (u->number > UNDEAD_MIN && !is_monsters(u->faction)
        && rng_int() % 100 < UNDEAD_BREAKUP) {
        int m;
        unit *u2;

        n = 0;
        for (m = u->number; m; m--) {
            if (rng_int() % 100 < UNDEAD_BREAKUP_FRACTION)
                ++n;
        }
        u2 = create_unit(r, get_monsters(), 0, get_race(RC_UNDEAD), 0, NULL, u);
        make_undead_unit(u2);
        transfermen(u, u2, u->number - n);
    }
}

void age_skeleton(unit * u)
{
    if (is_monsters(u->faction) && rng_int() % 100 < age_chance(u->age, 27, 1)) {
        int n = _max(1, u->number / 2);
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        u_setrace(u, get_race(RC_SKELETON_LORD));
        u->irace = NULL;
        scale_number(u, n);
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}

void age_zombie(unit * u)
{
    if (is_monsters(u->faction) && rng_int() % 100 < age_chance(u->age, 27, 1)) {
        int n = _max(1, u->number / 2);
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        u_setrace(u, get_race(RC_ZOMBIE_LORD));
        u->irace = NULL;
        scale_number(u, n);
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}

void age_ghoul(unit * u)
{
    if (is_monsters(u->faction) && rng_int() % 100 < age_chance(u->age, 27, 1)) {
        int n = _max(1, u->number / 2);
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        u_setrace(u, get_race(RC_GHOUL_LORD));
        u->irace = NULL;
        scale_number(u, n);
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}
