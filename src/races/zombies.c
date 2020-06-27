#include <platform.h>

/* kernel includes */
#include <kernel/race.h>
#include <kernel/order.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/region.h>

/* util iclude */
#include <util/rng.h>

#include "monsters.h"

/* libc includes */
#include <stdlib.h>

#define UNDEAD_MIN                  90  /* mind. zahl vor weg gehen */
#define UNDEAD_BREAKUP              25  /* chance dafuer */
#define UNDEAD_BREAKUP_FRACTION     (25+rng_int()%70)   /* anteil der weg geht */

static int age_chance(int a, int b, int p) {
    int r = (a - b) * p;
    return (r < 0) ? 0 : r;
}

void make_undead_unit(unit * u)
{
    free_orders(&u->orders);
    name_unit(u);
    u->flags |= UFL_ISNEW;
}

void age_skeleton(unit * u)
{
    if (is_monsters(u->faction) && rng_int() % 100 < age_chance(u->age, 27, 1)) {
        int n = u->number / 2;
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        if (n < 1) n = 1;
        u_setrace(u, get_race(RC_SKELETON_LORD));
        u->irace = NULL;
        scale_number(u, n);
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}

void age_zombie(unit * u)
{
    if (is_monsters(u->faction) && rng_int() % 100 < age_chance(u->age, 27, 1)) {
        int n = u->number / 2;
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        if (n < 1) n = 1;
        u_setrace(u, get_race(RC_ZOMBIE_LORD));
        u->irace = NULL;
        scale_number(u, n);
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}

void age_ghoul(unit * u)
{
    if (is_monsters(u->faction) && rng_int() % 100 < age_chance(u->age, 27, 1)) {
        int n = u->number / 2;
        double q = (double)u->hp / (double)(unit_max_hp(u) * u->number);
        u_setrace(u, get_race(RC_GHOUL_LORD));
        u->irace = NULL;
        scale_number(u, (n > 0) ? n : 1);
        u->hp = (int)(unit_max_hp(u) * u->number * q);
    }
}
