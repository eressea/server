#include "weapons.h"
#include "battle.h"

#include <kernel/unit.h>
#include <kernel/build.h>
#include <kernel/callbacks.h>
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
#include <limits.h>
#include <stdlib.h>

/* damage types */

static void report_special_attacks(const fighter *af, const item_type *itype)
{
    battle *b = af->side->battle;
    unit *au = af->unit;
    message *msg;
    int k = af->special.attacks;
    const weapon_type *wtype = resource2weapon(itype->rtype);

    if (wtype->skill == SK_CATAPULT) {
        msg = msg_message("usecatapult", "amount unit", k, au);
    }
    else {
        msg = msg_message("useflamingsword", "amount unit", k, au);
    }
    message_all(b, msg);
    msg_release(msg);
}

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
        return false;                /* if no enemy found, no use doing standarad attack */
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
    int shots = INT_MAX, d = 0, enemies;
    const resource_type *rtype;

    if (au->status >= ST_AVOID) {
        /* do not want to waste ammo. no shots fired */
        return false;
    }

    assert(wtype && wtype == WEAPON_TYPE(af->person[at->index].missile));
    assert(af->person[at->index].reload == 0);
    rtype = rt_find("catapultammo");

    if (rtype) {
        shots = get_pooled(au, rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, 1);
        if (shots <= 0) {
            return false;
        }
    }

    enemies = count_enemies(b, af, FIGHT_ROW, FIGHT_ROW, SELECT_ADVANCE);
    if (enemies > CATAPULT_ATTACKS) enemies = CATAPULT_ATTACKS;
    if (enemies == 0) {
        return false;
    }

    if (rtype) {
        use_pooled(au, rtype, GET_DEFAULT, 1);
    }

    while (enemies-- > 0) {
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
    callbacks.report_special_attacks = report_special_attacks;
    register_function((pf_generic)attack_catapult, "attack_catapult");
    register_function((pf_generic)attack_firesword, "attack_firesword");
}
