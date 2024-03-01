#include "alchemy.h"
#include "guard.h"
#include "study.h"

#include <kernel/attrib.h>
#include <kernel/build.h>
#include <kernel/faction.h>
#include <kernel/gamedata.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/region.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include "kernel/skill.h"
#include "kernel/unit.h"

/* util includes */
#include <util/base36.h>
#include <util/log.h>
#include <util/macros.h>
#include "util/message.h"
#include <util/rand.h>

#include <storage.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct effect_data {
    const struct item_type* type;
    int value;
} effect_data;

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

    ptype = (potion_type *)calloc(1, sizeof(potion_type));
    assert(ptype);
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
    int effsk = effskill(u, SK_HERBALISM, NULL);
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
    rsetherbs(r, rherbs(r) - herbsfound);

    if (herbsfound) {
        produceexp(u, SK_HERBALISM);
        i_change(&u->items, whichherb, herbsfound);
        ADDMSG(&u->faction->msgs, msg_message("herbfound",
            "unit region amount herb", u, r, herbsfound, whichherb->rtype));
    }
    else {
        ADDMSG(&u->faction->msgs, msg_message("researchherb_none",
            "unit region", u, r));
    }
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

static int potion_luck(unit *u, region *r, attrib_type *atype, int amount) {
    attrib *a = (attrib *)a_find(r->attribs, atype);
    UNUSED_ARG(u);
    if (!a) {
        a = a_add(&r->attribs, a_new(atype));
    }
    a->data.i += amount;
    return amount;
}

int use_potion(unit * u, const item_type * itype, int amount, struct order *ord)
{
    region *r = u->region;

    if (itype == oldpotiontype[P_PEOPLE]) {
        amount = potion_luck(u, r, &at_peasantluck, amount);
    }
    else if (itype == oldpotiontype[P_HORSE]) {
        amount = potion_luck(u, r, &at_horseluck, amount);
    }
    else {
        change_effect(u, itype, 10 * amount);
    }
    if (amount > 0) {
        ADDMSG(&u->faction->msgs, msg_message("use_item",
            "unit amount item", u, amount, itype->rtype));
    }
    return amount;
}

/*****************/
/*   at_effect   */
/*****************/

static void a_initeffect(variant *var)
{
    var->v = calloc(1, sizeof(effect_data));
}

static void
a_writeeffect(const variant *var, const void *owner, struct storage *store)
{
    effect_data *edata = (effect_data *)var->v;
    UNUSED_ARG(owner);
    WRITE_TOK(store, resourcename(edata->type->rtype, 0));
    WRITE_INT(store, edata->value);
}

static int a_readeffect(variant *var, void *owner, struct gamedata *data)
{
    struct storage *store = data->store;
    int power;
    const resource_type *rtype;
    effect_data *edata = (effect_data *)var->v;
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
    a_free_voidptr,
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
    int skill = effskill(u, SK_ALCHEMY, NULL);
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

void transfer_effects(const unit* u, unit* dst, int n)
{
    attrib * a = a_find(u->attribs, &at_effect);
    while (a && a->type == &at_effect) {
        effect_data* olde = (effect_data*)a->data.v;
        int delta = olde->value * n;
        if (delta >= u->number) {
            delta = delta / u->number;
            olde->value -= delta;
            change_effect(dst, olde->type, delta);
        }
        a = a->next;
    }
}

int effect_value(const struct attrib* a)
{
    const effect_data* data = (const effect_data*)a->data.v;
    return data->value;
}

const struct item_type* effect_type(const struct attrib* a)
{
    const effect_data* data = (const effect_data*)a->data.v;
    return data->type;
}

void scale_effects(attrib* alist, int n, int size)
{
    const attrib* a = a_find(alist, &at_effect);

    for (; a && a->type == &at_effect; a = a->next) {
        effect_data* data = (effect_data*)a->data.v;
        data->value = (long long)data->value * n / size;
    }
}

int use_foolpotion(unit *user, const item_type *itype, int amount,
    struct order *ord)
{
    int targetno = read_unitid(user->faction, user->region);
    unit *u = findunit(targetno);
    int max_effects;
    if (u == NULL || user->region != u->region) {
        ADDMSG(&user->faction->msgs, msg_feedback(user, ord, "feedback_unit_not_found",
            ""));
        return ECUSTOM;
    }
    if (effskill(user, SK_STEALTH, NULL) <= effskill(u, SK_PERCEPTION, NULL)) {
        cmistake(user, ord, 64, MSG_EVENT);
        return ECUSTOM;
    }
    max_effects = u->number * 10 - get_effect(u, itype);
    if (max_effects > 0) {
        int use = (max_effects + 9) / 10;
        int effects = max_effects;
        if (use > amount) {
            use = amount;
            effects = use * 10;
        }
        change_effect(u, itype, effects);
        ADDMSG(&user->faction->msgs, msg_message("givedumb",
            "unit recipient amount", user, u, use));
        return use;
    }
    return 0;
}

