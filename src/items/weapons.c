#include "weapons.h"
#include "battle.h"

#include <kernel/unit.h>
#include <kernel/build.h>
#include <kernel/config.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/pool.h>

/* util includes */
#include <util/functions.h>
#include <util/message.h>
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
            const weapon *wp = fi->person[i].melee;
            if (wp != NULL && wp->item->type == wtype->itype)
                ++k;
        }
        msg = msg_message("useflamingsword", "amount unit", k, fi->unit);
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
    if (killed > 0 && casualties)
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
    const resource_type *rtype;

    if (au->status >= ST_AVOID) {
        /* do not want to waste ammo. no shots fired */
        return false;
    }

    assert(wtype == WEAPON_TYPE(af->person[at->index].missile));
    assert(af->person[at->index].reload == 0);
    rtype = rt_find("catapultammo");

    if (rtype) {
        if (get_pooled(au, rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, 1) <= 0) {
            return false;
        }
    }

    enemies = count_enemies(b, af, FIGHT_ROW, FIGHT_ROW, SELECT_ADVANCE);
    if (enemies > CATAPULT_ATTACKS) enemies = CATAPULT_ATTACKS;
    if (enemies == 0) {
        return false;
    }

    if (af->catmsg == -1) {
        int i, k = 0;
        message *msg;

        for (i = 0; i <= at->index; ++i) {
            if (af->person[i].reload == 0 && af->person[i].missile->item->type == wtype->itype)
                ++k;
        }
        msg = msg_message("usecatapult", "amount unit", k, au);
        message_all(b, msg);
        msg_release(msg);
        af->catmsg = 0;
    }

    if (rtype) {
        use_pooled(au, rtype, GET_DEFAULT, 1);
    }

    while (--enemies >= 0) {
        /* Select defender */
        dt = select_enemy(af, FIGHT_ROW, FIGHT_ROW, SELECT_ADVANCE);
        if (!dt.fighter)
            break;

        /* If battle succeeds */
        if (hits(*at, dt, wtype)) {
            int chance_pct = config_get_int("rules.catapult.damage.chance_percent", 5);
            d += terminate(dt, *at, AT_STANDARD, wtype->damage[0], true);
            structural_damage(dt, 0, chance_pct);
        }
    }

    if (casualties) {
        *casualties = d;
    }
    return true; /* catapult must reload */
}

void register_weapons(void)
{
    register_function((pf_generic)attack_catapult, "attack_catapult");
    register_function((pf_generic)attack_firesword, "attack_firesword");
}
