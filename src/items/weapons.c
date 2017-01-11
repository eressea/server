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
#include "weapons.h"
#include "battle.h"

#include <kernel/unit.h>
#include <kernel/build.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/pool.h>

/* util includes */
#include <util/functions.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>

/* damage types */

static bool
attack_firesword(const troop * at, const struct weapon_type *wtype,
int *casualties)
{
    fighter *fi = at->fighter;
    troop dt;
    int killed = 0;
    const char *damage = "2d8";
    int force = 1 + rng_int() % 10;
    int enemies =
        count_enemies(fi->side->battle, fi, 0, 1, SELECT_ADVANCE | SELECT_DISTANCE);

    if (!enemies) {
        if (casualties)
            *casualties = 0;
        return true;                /* if no enemy found, no use doing standarad attack */
    }

    if (fi->catmsg == -1) {
        int i, k = 0;
        message *msg;
        for (i = 0; i <= at->index; ++i) {
            struct weapon *wp = fi->person[i].melee;
            if (wp != NULL && wp->type == wtype)
                ++k;
        }
        msg = msg_message("battle::useflamingsword", "amount unit", k, fi->unit);
        message_all(fi->side->battle, msg);
        msg_release(msg);
        fi->catmsg = 0;
    }

    do {
        dt = select_enemy(fi, 0, 1, SELECT_ADVANCE | SELECT_DISTANCE);
        --force;
        if (dt.fighter) {
            killed += terminate(dt, *at, AT_SPELL, damage, 1);
        }
    } while (force && killed < enemies);
    if (casualties)
        *casualties = killed;
    return true;
}

#define CATAPULT_ATTACKS 6

static bool
attack_catapult(const troop * at, const struct weapon_type *wtype,
int *casualties)
{
    fighter *af = at->fighter;
    unit *au = af->unit;
    battle *b = af->side->battle;
    troop dt;
    int d = 0, enemies;
    weapon *wp = af->person[at->index].missile;
    const resource_type *rtype = rt_find("catapultammo");

    assert(wp->type == wtype);
    assert(af->person[at->index].reload == 0);

    if (rtype) {
        if (get_pooled(au, rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, 1) <= 0) {
            /* No ammo. Use other weapon if available. */
            return true;
        }
    }

    enemies = count_enemies(b, af, FIGHT_ROW, FIGHT_ROW, SELECT_ADVANCE);
    enemies = MIN(enemies, CATAPULT_ATTACKS);
    if (enemies == 0) {
        return true;                /* allow further attacks */
    }

    if (af->catmsg == -1) {
        int i, k = 0;
        message *msg;

        for (i = 0; i <= at->index; ++i) {
            if (af->person[i].reload == 0 && af->person[i].missile == wp)
                ++k;
        }
        msg = msg_message("battle::usecatapult", "amount unit", k, au);
        message_all(b, msg);
        msg_release(msg);
        af->catmsg = 0;
    }

    if (rtype) {
        use_pooled(au, rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, 1);
    }

    while (--enemies >= 0) {
        /* Select defender */
        dt = select_enemy(af, FIGHT_ROW, FIGHT_ROW, SELECT_ADVANCE);
        if (!dt.fighter)
            break;

        /* If battle succeeds */
        if (hits(*at, dt, wp)) {
            d += terminate(dt, *at, AT_STANDARD, wp->type->damage[0], true);
#ifdef CATAPULT_STRUCTURAL_DAMAGE
            if (dt.fighter->unit->building && rng_int() % 100 < 5) {
                double dmg = config_get_flt("rules.building.damage.catapult", 1);
                damage_building(b, dt.fighter->unit->building, dmg);
            }
            else if (dt.fighter->unit->ship && rng_int() % 100 < 5) {
                double dmg = config_get_flt("rules.ship.damage.catapult", 0.01);
                damage_ship(dt.fighter->unit->ship, dmg)
            }
#endif
        }
    }

    if (casualties) {
        *casualties = d;
    }
    return false;                 /* keine weitren attacken */
}

void register_weapons(void)
{
    register_function((pf_generic)attack_catapult, "attack_catapult");
    register_function((pf_generic)attack_firesword, "attack_firesword");
}
