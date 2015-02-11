/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include "monster.h"

/* gamecode includes */
#include "economy.h"
#include "give.h"
#include "move.h"

/* triggers includes */
#include <triggers/removecurse.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/hate.h>

/* kernel includes */
#include <kernel/build.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pathfinder.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MOVECHANCE                  25  /* chance fuer bewegung */

#define MAXILLUSION_TEXTS   3

bool monster_is_waiting(const unit * u)
{
    if (fval(u, UFL_ISNEW | UFL_MOVED))
        return true;
    return false;
}

static void eaten_by_monster(unit * u)
{
    /* adjustment for smaller worlds */
    static double multi = 0.0;
    int n = 0;
    int horse = -1;
    const resource_type *rhorse = get_resourcetype(R_HORSE);
    if (multi == 0.0) {
        multi = RESOURCE_QUANTITY * newterrain(T_PLAIN)->size / 10000.0;
    }

    switch (old_race(u_race(u))) {
    case RC_FIREDRAGON:
        n = rng_int() % 80 * u->number;
        break;
    case RC_DRAGON:
        n = rng_int() % 200 * u->number;
        break;
    case RC_WYRM:
        n = rng_int() % 500 * u->number;
        break;
    default:
        n = rng_int() % (u->number / 20 + 1);
        horse = 0;
    }
    horse = horse ? i_get(u->items, rhorse->itype) : 0;

    n = (int)(n * multi);
    if (n > 0) {
        n = lovar(n);
        n = _min(rpeasants(u->region), n);

        if (n > 0) {
            deathcounts(u->region, n);
            rsetpeasants(u->region, rpeasants(u->region) - n);
            ADDMSG(&u->region->msgs, msg_message("eatpeasants", "unit amount", u, n));
        }
    }
    if (horse > 0) {
        i_change(&u->items, rhorse->itype, -horse);
        ADDMSG(&u->region->msgs, msg_message("eathorse", "unit amount", u, horse));
    }
}

static void absorbed_by_monster(unit * u)
{
    int n;

    switch (old_race(u_race(u))) {
    default:
        n = rng_int() % (u->number / 20 + 1);
    }

    if (n > 0) {
        n = lovar(n);
        n = _min(rpeasants(u->region), n);
        if (n > 0) {
            rsetpeasants(u->region, rpeasants(u->region) - n);
            scale_number(u, u->number + n);
            ADDMSG(&u->region->msgs, msg_message("absorbpeasants",
                "unit race amount", u, u_race(u), n));
        }
    }
}

static int scareaway(region * r, int anzahl)
{
    int n, p, diff = 0, emigrants[MAXDIRECTIONS];
    direction_t d;

    anzahl = _min(_max(1, anzahl), rpeasants(r));

    /* Wandern am Ende der Woche (normal) oder wegen Monster. Die
     * Wanderung wird erst am Ende von demographics () ausgefuehrt.
     * emigrants[] ist local, weil r->newpeasants durch die Monster
     * vielleicht schon hochgezaehlt worden ist. */

    for (d = 0; d != MAXDIRECTIONS; d++)
        emigrants[d] = 0;

    p = rpeasants(r);
    assert(p >= 0 && anzahl >= 0);
    for (n = _min(p, anzahl); n; n--) {
        direction_t dir = (direction_t)(rng_int() % MAXDIRECTIONS);
        region *rc = rconnect(r, dir);

        if (rc && fval(rc->terrain, LAND_REGION)) {
            ++diff;
            rc->land->newpeasants++;
            emigrants[dir]++;
        }
    }
    rsetpeasants(r, p - diff);
    assert(p >= diff);
    return diff;
}

static void scared_by_monster(unit * u)
{
    int n;

    switch (old_race(u_race(u))) {
    case RC_FIREDRAGON:
        n = rng_int() % 160 * u->number;
        break;
    case RC_DRAGON:
        n = rng_int() % 400 * u->number;
        break;
    case RC_WYRM:
        n = rng_int() % 1000 * u->number;
        break;
    default:
        n = rng_int() % (u->number / 4 + 1);
    }

    if (n > 0) {
        n = lovar(n);
        n = _min(rpeasants(u->region), n);
        if (n > 0) {
            n = scareaway(u->region, n);
            if (n > 0) {
                ADDMSG(&u->region->msgs, msg_message("fleescared",
                    "amount unit", n, u));
            }
        }
    }
}

void monster_kills_peasants(unit * u)
{
    if (!monster_is_waiting(u)) {
        if (u_race(u)->flags & RCF_SCAREPEASANTS) {
            scared_by_monster(u);
        }
        if (u_race(u)->flags & RCF_KILLPEASANTS) {
            eaten_by_monster(u);
        }
        if (u_race(u)->flags & RCF_ABSORBPEASANTS) {
            absorbed_by_monster(u);
        }
    }
}

faction *get_or_create_monsters(void)
{
    faction *f = findfaction(MONSTER_ID);
    if (!f) {
        const race *rc = rc_get_or_create("dragon");
        const char *email = get_param(global.parameters, "monster.email");
        f = addfaction(email ? email : "noreply@eressea.de", NULL, rc, default_locale, 0);
        renumber_faction(f, MONSTER_ID);
        faction_setname(f, "Monster");
        fset(f, FFL_NPC | FFL_NOIDLEOUT);
    }
    return f;
}

faction *get_monsters(void) {
    return get_or_create_monsters();
}

void make_zombie(unit * u)
{
    u_setfaction(u, get_monsters());
    scale_number(u, 1);
    u->hp = unit_max_hp(u) * u->number;
    u_setrace(u, get_race(RC_ZOMBIE));
    u->irace = NULL;
}
