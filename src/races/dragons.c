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
    u->hp = unit_max_hp(u);
    u_setrace(u, rc);
    u->irace = NULL;
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
