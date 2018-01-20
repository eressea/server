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
#include "guard.h"
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
#include <util/macros.h>
#include <util/rand.h>

#include <storage.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct potion_type {
    struct potion_type *next;
    const struct item_type *itype;
    int level;
} potion_type;

static potion_type *potiontypes;

static void pt_register(potion_type * ptype)
{
    ptype->next = potiontypes;
    potiontypes = ptype;
}

void new_potiontype(item_type * itype, int level)
{
    potion_type *ptype;

    ptype = (potion_type *)calloc(sizeof(potion_type), 1);
    itype->flags |= ITF_POTION;
    ptype->itype = itype;
    ptype->level = level;
    pt_register(ptype);
}

int potion_level(const item_type *itype)
{
    potion_type *ptype;
    for (ptype = potiontypes; ptype; ptype = ptype->next) {
        if (ptype->itype == itype) {
            return ptype->level;
        }
    }
    return 0;
}

void herbsearch(unit * u, int max_take)
{
    region * r = u->region;
    int herbsfound;
    const item_type *whichherb;
    int effsk = effskill(u, SK_HERBALISM, 0);
    int herbs = rherbs(r);

    if (effsk == 0) {
        cmistake(u, u->thisorder, 59, MSG_PRODUCE);
        return;
    }

    if (is_guarded(r, u)) {
        cmistake(u, u->thisorder, 70, MSG_EVENT);
        return;
    }

    whichherb = rherbtype(r);
    if (whichherb == NULL) {
        cmistake(u, u->thisorder, 108, MSG_PRODUCE);
        return;
    }

    if (max_take < herbs) {
        herbs = max_take;
    }
    herbsfound = ntimespprob(effsk * u->number,
        (double)rherbs(r) / 100.0F, -0.01F);

    if (herbsfound > herbs) herbsfound = herbs;
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

static int begin_potion(unit * u, const item_type * itype, struct order *ord)
{
    static int config;
    static bool rule_multipotion;

    assert(itype);
    if (config_changed(&config)) {
        /* should we allow multiple different potions to be used the same turn? */
        rule_multipotion = config_get_int("rules.magic.multipotion", 0) != 0;
    }

    if (!rule_multipotion) {
        const item_type *use = ugetpotionuse(u);
        if (use != NULL && use != itype) {
            ADDMSG(&u->faction->msgs,
                msg_message("errusingpotion", "unit using command",
                u, use->rtype, ord));
            return ECUSTOM;
        }
    }
    return 0;
}

static void end_potion(unit * u, const item_type * itype, int amount)
{
    use_pooled(u, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(u, itype);

    ADDMSG(&u->faction->msgs, msg_message("usepotion",
        "unit potion", u, itype->rtype));
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

void show_potions(faction *f, int sklevel)
{
    const potion_type *ptype;
    for (ptype = potiontypes; ptype; ptype = ptype->next) {
        if (ptype->level > 0 && sklevel == ptype->level * 2) {
            attrib *a = a_find(f->attribs, &at_showitem);
            while (a && a->type == &at_showitem && a->data.v != ptype)
                a = a->next;
            if (a == NULL || a->type != &at_showitem) {
                a = a_add(&f->attribs, a_new(&at_showitem));
                a->data.v = (void *)ptype->itype;
            }
        }
    }
}

static int potion_healing(unit * u, int amount) {
    int maxhp = unit_max_hp(u) * u->number;
    u->hp = u->hp + 400 * amount;
    if (u->hp > maxhp) u->hp = maxhp;
    return amount;
}

static int potion_luck(unit *u, region *r, attrib_type *atype, int amount) {
    attrib *a = (attrib *)a_find(r->attribs, atype);
    UNUSED_ARG(u);
    if (!a) {
        a = a_add(&r->attribs, a_new(atype));
    }
    a->data.i += amount;
    return amount;
}

static int potion_power(unit *u, int amount) {
    int hp = 10 * amount;

    if (hp > u->number) {
        hp = u->number;
        amount = (hp + 9) % 10;
    }
    u->hp += hp * unit_max_hp(u) * 4;
    return amount;
}

static int do_potion(unit * u, region *r, const item_type * itype, int amount)
{
    if (itype == oldpotiontype[P_LIFE]) {
        return potion_water_of_life(u, r, amount);
    }
    else if (itype == oldpotiontype[P_HEILWASSER]) {
        return potion_healing(u, amount);
    }
    else if (itype == oldpotiontype[P_PEOPLE]) {
        return potion_luck(u, r, &at_peasantluck, amount);
    }
    else if (itype == oldpotiontype[P_HORSE]) {
        return potion_luck(u, r, &at_horseluck, amount);
    }
    else if (itype == oldpotiontype[P_MACHT]) {
        return potion_power(u, amount);
    }
    else {
        change_effect(u, itype, 10 * amount);
    }
    return amount;
}

int use_potion(unit * u, const item_type * itype, int amount, struct order *ord)
{
    if (oldpotiontype[P_HEAL] && itype == oldpotiontype[P_HEAL]) {
        return EUNUSABLE;
    }
    else {
        int result = begin_potion(u, itype, ord);
        if (result)
            return result;
        amount = do_potion(u, u->region, itype, amount);
        end_potion(u, itype, amount);
    }
    return 0;
}

typedef struct potiondelay {
    unit *u;
    region *r;
    const item_type *itype;
    int amount;
} potiondelay;

static void init_potiondelay(attrib * a)
{
    a->data.v = malloc(sizeof(potiondelay));
}

static void free_potiondelay(attrib * a) {
    free(a->data.v);
}

static int age_potiondelay(attrib * a, void *owner)
{
    potiondelay *pd = (potiondelay *)a->data.v;
    UNUSED_ARG(owner);
    pd->amount = do_potion(pd->u, pd->r, pd->itype, pd->amount);
    return AT_AGE_REMOVE;
}

attrib_type at_potiondelay = {
    "potiondelay",
    init_potiondelay,
    free_potiondelay,
    age_potiondelay, 0, 0
};

static attrib *make_potiondelay(unit * u, const item_type * itype, int amount)
{
    attrib *a = a_new(&at_potiondelay);
    potiondelay *pd = (potiondelay *)a->data.v;
    pd->u = u;
    pd->r = u->region;
    pd->itype = itype;
    pd->amount = amount;
    return a;
}

int
use_potion_delayed(unit * u, const item_type * itype, int amount,
struct order *ord)
{
    int result = begin_potion(u, itype, ord);
    if (result)
        return result;

    a_add(&u->attribs, make_potiondelay(u, itype, amount));

    end_potion(u, itype, amount);
    return 0;
}

/*****************/
/*   at_effect   */
/*****************/

static void a_initeffect(attrib * a)
{
    a->data.v = calloc(sizeof(effect_data), 1);
}

static void a_finalizeeffect(attrib * a) /*-V524 */
{
    free(a->data.v);
}

static void
a_writeeffect(const attrib * a, const void *owner, struct storage *store)
{
    effect_data *edata = (effect_data *)a->data.v;
    UNUSED_ARG(owner);
    WRITE_TOK(store, resourcename(edata->type->rtype, 0));
    WRITE_INT(store, edata->value);
}

static int a_readeffect(attrib * a, void *owner, struct gamedata *data)
{
    struct storage *store = data->store;
    int power;
    const resource_type *rtype;
    effect_data *edata = (effect_data *)a->data.v;
    char zText[32];

    UNUSED_ARG(owner);
    READ_TOK(store, zText, sizeof(zText));
    rtype = rt_find(zText);

    READ_INT(store, &power);
    if (rtype == NULL || rtype->itype == NULL || power <= 0) {
        return AT_READ_FAIL;
    }
    if (data->version < NOLANDITEM_VERSION) {
        if (rtype->itype == oldpotiontype[P_HEAL]) {
            /* healing potions used to have long-term effects */
            return AT_READ_FAIL;
        }
    }
    edata->type = rtype->itype;
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

int get_effect(const unit * u, const item_type * effect)
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

int change_effect(unit * u, const item_type * effect, int delta)
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

bool display_potions(struct unit *u)
{
    int skill = effskill(u, SK_ALCHEMY, 0);
    int c = 0;
    const potion_type *ptype;
    for (ptype = potiontypes; ptype != NULL; ptype = ptype->next) {
        if (ptype->level * 2 <= skill) {
            show_item(u, ptype->itype);
            ++c;
        }
    }
    return (c > 0);
}
