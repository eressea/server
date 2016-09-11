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
#include "alchemy.h"
#include "move.h"
#include "skill.h"
#include "study.h"

#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/build.h>
#include <kernel/region.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/gamedata.h>
#include <util/base36.h>
#include <util/log.h>
#include <util/rand.h>

#include <storage.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------------------------------- */

void herbsearch(unit * u, int max)
{
    region * r = u->region;
    int herbsfound;
    const item_type *whichherb;
    int effsk = effskill(u, SK_HERBALISM, 0);

    if (effsk == 0) {
        cmistake(u, u->thisorder, 59, MSG_PRODUCE);
        return;
    }

    if (is_guarded(r, u, GUARD_PRODUCE)) {
        cmistake(u, u->thisorder, 70, MSG_EVENT);
        return;
    }

    whichherb = rherbtype(r);
    if (whichherb == NULL) {
        cmistake(u, u->thisorder, 108, MSG_PRODUCE);
        return;
    }

    if (max)
        max = _min(max, rherbs(r));
    else
        max = rherbs(r);
    herbsfound = ntimespprob(effsk * u->number,
        (double)rherbs(r) / 100.0F, -0.01F);
    herbsfound = _min(herbsfound, max);
    rsetherbs(r, (short) (rherbs(r) - herbsfound));

    if (herbsfound) {
        produceexp(u, SK_HERBALISM, u->number);
        i_change(&u->items, whichherb, herbsfound);
        ADDMSG(&u->faction->msgs, msg_message("herbfound",
            "unit region amount herb", u, r, herbsfound, whichherb->rtype));
    }
    else {
        ADDMSG(&u->faction->msgs, msg_message("researchherb_none",
            "unit region", u, r));
    }
}

static int begin_potion(unit * u, const potion_type * ptype, struct order *ord)
{
    bool rule_multipotion;
    assert(ptype != NULL);

    /* should we allow multiple different potions to be used the same turn? */
    rule_multipotion = config_get_int("rules.magic.multipotion", 0) != 0;
    if (!rule_multipotion) {
        const potion_type *use = ugetpotionuse(u);
        if (use != NULL && use != ptype) {
            ADDMSG(&u->faction->msgs,
                msg_message("errusingpotion", "unit using command",
                u, use->itype->rtype, ord));
            return ECUSTOM;
        }
    }
    return 0;
}

static void end_potion(unit * u, const potion_type * ptype, int amount)
{
    use_pooled(u, ptype->itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(u, ptype);

    ADDMSG(&u->faction->msgs, msg_message("usepotion",
        "unit potion", u, ptype->itype->rtype));
}

static int potion_water_of_life(unit * u, region *r, int amount) {
    static int config;
    static int tree_type, tree_count;
    int wood = 0;

    if (config_changed(&config)) {
        tree_type = config_get_int("rules.magic.wol_type", 1);
        tree_count = config_get_int("rules.magic.wol_effect", 10);
    }
    /* mallorn is required to make mallorn forests, wood for regular ones */
    if (fval(r, RF_MALLORN)) {
        wood = use_pooled(u, rt_find("mallorn"),
            GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, tree_count * amount);
    }
    else {
        wood = use_pooled(u, rt_find("log"),
            GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, tree_count * amount);
    }
    if (r->land == 0)
        wood = 0;
    if (wood < tree_count * amount) {
        int x = wood / tree_count;
        if (wood % tree_count)
            ++x;
        if (x < amount)
            amount = x;
    }
    rsettrees(r, tree_type, rtrees(r, tree_type) + wood);
    ADDMSG(&u->faction->msgs, msg_message("growtree_effect",
        "mage amount", u, wood));
    return amount;
}

static int potion_healing(unit * u, int amount) {
    u->hp = _min(unit_max_hp(u) * u->number, u->hp + 400 * amount);
    return amount;
}

static int potion_luck(unit *u, region *r, attrib_type *atype, int amount) {
    attrib *a = (attrib *)a_find(r->attribs, atype);
    if (!a) {
        a = a_add(&r->attribs, a_new(atype));
    }
    a->data.i += amount;
    return amount;
}

static int potion_truth(unit *u) {
    fset(u, UFL_DISBELIEVES);
    return 1;
}

static int potion_power(unit *u, int amount) {
    int use = u->number / 10;
    if (use < amount) {
        if (u->number % 10 > 0) ++use;
        amount = use;
    }
    /* Verfünffacht die HP von max. 10 Personen in der Einheit */
    u->hp += _min(u->number, 10 * amount) * unit_max_hp(u) * 4;
    return amount;
}

static int do_potion(unit * u, region *r, const potion_type * ptype, int amount)
{
    if (ptype == oldpotiontype[P_LIFE]) {
        return potion_water_of_life(u, r, amount);
    }
    else if (ptype == oldpotiontype[P_HEILWASSER]) {
        return potion_healing(u, amount);
    }
    else if (ptype == oldpotiontype[P_PEOPLE]) {
        return potion_luck(u, r, &at_peasantluck, amount);
    }
    else if (ptype == oldpotiontype[P_HORSE]) {
        return potion_luck(u, r, &at_horseluck, amount);
    }
    else if (ptype == oldpotiontype[P_WAHRHEIT]) {
        return potion_truth(u);
    }
    else if (ptype == oldpotiontype[P_MACHT]) {
        return potion_power(u, amount);
    }
    else {
        change_effect(u, ptype, 10 * amount);
    }
    return amount;
}

int use_potion(unit * u, const item_type * itype, int amount, struct order *ord)
{
    const potion_type *ptype = resource2potion(itype->rtype);

    if (oldpotiontype[P_HEAL] && ptype == oldpotiontype[P_HEAL]) {
        return EUNUSABLE;
    }
    else {
        int result = begin_potion(u, ptype, ord);
        if (result)
            return result;
        amount = do_potion(u, u->region, ptype, amount);
        end_potion(u, ptype, amount);
    }
    return 0;
}

typedef struct potiondelay {
    unit *u;
    region *r;
    const potion_type *ptype;
    int amount;
} potiondelay;

static void init_potiondelay(attrib * a)
{
    a->data.v = malloc(sizeof(potiondelay));
}

static void free_potiondelay(attrib * a)
{
    free(a->data.v);
}

static int age_potiondelay(attrib * a, void *owner)
{
    potiondelay *pd = (potiondelay *)a->data.v;
    unused_arg(owner);
    pd->amount = do_potion(pd->u, pd->r, pd->ptype, pd->amount);
    return AT_AGE_REMOVE;
}

attrib_type at_potiondelay = {
    "potiondelay",
    init_potiondelay,
    free_potiondelay,
    age_potiondelay, 0, 0
};

static attrib *make_potiondelay(unit * u, const potion_type * ptype, int amount)
{
    attrib *a = a_new(&at_potiondelay);
    potiondelay *pd = (potiondelay *)a->data.v;
    pd->u = u;
    pd->r = u->region;
    pd->ptype = ptype;
    pd->amount = amount;
    return a;
}

int
use_potion_delayed(unit * u, const item_type * itype, int amount,
struct order *ord)
{
    const potion_type *ptype = resource2potion(itype->rtype);
    int result = begin_potion(u, ptype, ord);
    if (result)
        return result;

    a_add(&u->attribs, make_potiondelay(u, ptype, amount));

    end_potion(u, ptype, amount);
    return 0;
}

/*****************/
/*   at_effect   */
/*****************/

static void a_initeffect(attrib * a)
{
    a->data.v = calloc(sizeof(effect_data), 1);
}

static void a_finalizeeffect(attrib * a)
{
    free(a->data.v);
}

static void
a_writeeffect(const attrib * a, const void *owner, struct storage *store)
{
    effect_data *edata = (effect_data *)a->data.v;
    WRITE_TOK(store, resourcename(edata->type->itype->rtype, 0));
    WRITE_INT(store, edata->value);
}

static int a_readeffect(attrib * a, void *owner, struct gamedata *data)
{
    struct storage *store = data->store;
    int power;
    const resource_type *rtype;
    effect_data *edata = (effect_data *)a->data.v;
    char zText[32];

    READ_TOK(store, zText, sizeof(zText));
    rtype = rt_find(zText);

    READ_INT(store, &power);
    if (rtype == NULL || rtype->ptype == NULL || power <= 0) {
        return AT_READ_FAIL;
    }
    if (rtype->ptype==oldpotiontype[P_HEAL]) {
        // healing potions used to have long-term effects
        return AT_READ_FAIL;
    }
    edata->type = rtype->ptype;
    edata->value = power;
    return AT_READ_OK;
}

attrib_type at_effect = {
    "effect",
    a_initeffect,
    a_finalizeeffect,
    DEFAULT_AGE,
    a_writeeffect,
    a_readeffect,
};

int get_effect(const unit * u, const potion_type * effect)
{
    const attrib *a;
    for (a = a_find(u->attribs, &at_effect); a != NULL && a->type == &at_effect;
        a = a->next) {
        const effect_data *data = (const effect_data *)a->data.v;
        if (data->type == effect)
            return data->value;
    }
    return 0;
}

int change_effect(unit * u, const potion_type * effect, int delta)
{
    if (delta != 0) {
        attrib *a = a_find(u->attribs, &at_effect);
        effect_data *data = NULL;

        while (a && a->type == &at_effect) {
            data = (effect_data *)a->data.v;
            if (data->type == effect) {
                if (data->value + delta == 0) {
                    a_remove(&u->attribs, a);
                    return 0;
                }
                else {
                    data->value += delta;
                    return data->value;
                }
            }
            a = a->next;
        }

        a = a_add(&u->attribs, a_new(&at_effect));
        data = (effect_data *)a->data.v;
        data->type = effect;
        data->value = delta;
        return data->value;
    }
    log_error("change effect with delta==0 for unit %s\n", itoa36(u->no));
    return 0;
}
