/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

void herbsearch(region * r, unit * u, int max)
{
  int herbsfound;
  const item_type *whichherb;

  if (eff_skill(u, SK_HERBALISM, r) == 0) {
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
  herbsfound = ntimespprob(eff_skill(u, SK_HERBALISM, r) * u->number,
    (double)rherbs(r) / 100.0F, -0.01F);
  herbsfound = _min(herbsfound, max);
  rsetherbs(r, rherbs(r) - herbsfound);

  if (herbsfound) {
    produceexp(u, SK_HERBALISM, u->number);
    i_change(&u->items, whichherb, herbsfound);
    ADDMSG(&u->faction->msgs, msg_message("herbfound",
        "unit region amount herb", u, r, herbsfound, whichherb->rtype));
  } else {
    ADDMSG(&u->faction->msgs, msg_message("researchherb_none",
        "unit region", u, u->region));
  }
}

static int begin_potion(unit * u, const potion_type * ptype, struct order *ord)
{
  static int rule_multipotion = -1;
  assert(ptype != NULL);

  if (rule_multipotion < 0) {
    /* should we allow multiple different potions to be used the same turn? */
    rule_multipotion =
      get_param_int(global.parameters, "rules.magic.multipotion", 0);
  }
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

static int do_potion(unit * u, region *r, const potion_type * ptype, int amount)
{
  if (ptype == oldpotiontype[P_LIFE]) {
    int holz = 0;
    static int tree_type = -1;
    static int tree_count = -1;
    if (tree_type < 0) {
      tree_type = get_param_int(global.parameters, "rules.magic.wol_type", 1);
      tree_count =
        get_param_int(global.parameters, "rules.magic.wol_effect", 10);
    }
    /* mallorn is required to make mallorn forests, wood for regular ones */
    if (fval(r, RF_MALLORN)) {
      holz = use_pooled(u, rt_find("mallorn"),
        GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, tree_count * amount);
    } else {
      holz = use_pooled(u, rt_find("log"),
        GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, tree_count * amount);
    }
    if (r->land == 0)
      holz = 0;
    if (holz < tree_count * amount) {
      int x = holz / tree_count;
      if (holz % tree_count)
        ++x;
      if (x < amount)
        amount = x;
    }
    rsettrees(r, tree_type, rtrees(r, tree_type) + holz);
    ADDMSG(&u->faction->msgs, msg_message("growtree_effect",
        "mage amount", u, holz));
  } else if (ptype == oldpotiontype[P_HEILWASSER]) {
    u->hp = _min(unit_max_hp(u) * u->number, u->hp + 400 * amount);
  } else if (ptype == oldpotiontype[P_PEOPLE]) {
    attrib *a = (attrib *) a_find(r->attribs, &at_peasantluck);
    if (!a)
      a = a_add(&r->attribs, a_new(&at_peasantluck));
    a->data.i += amount;
  } else if (ptype == oldpotiontype[P_HORSE]) {
    attrib *a = (attrib *) a_find(r->attribs, &at_horseluck);
    if (!a)
      a = a_add(&r->attribs, a_new(&at_horseluck));
    a->data.i += amount;
  } else if (ptype == oldpotiontype[P_WAHRHEIT]) {
    fset(u, UFL_DISBELIEVES);
    amount = 1;
  } else if (ptype == oldpotiontype[P_MACHT]) {
    /* Verf�nffacht die HP von max. 10 Personen in der Einheit */
    u->hp += _min(u->number, 10 * amount) * unit_max_hp(u) * 4;
  } else {
    change_effect(u, ptype, 10 * amount);
  }
  return amount;
}

int use_potion(unit * u, const item_type * itype, int amount, struct order *ord)
{
  const potion_type *ptype = resource2potion(itype->rtype);

  if (oldpotiontype[P_HEAL] && ptype == oldpotiontype[P_HEAL]) {
    return EUNUSABLE;
  } else {
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

static int age_potiondelay(attrib * a)
{
  potiondelay *pd = (potiondelay *) a->data.v;
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
  potiondelay *pd = (potiondelay *) a->data.v;
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
  effect_data *edata = (effect_data *) a->data.v;
  WRITE_TOK(store, resourcename(edata->type->itype->rtype, 0));
  WRITE_INT(store, edata->value);
}

static int a_readeffect(attrib * a, void *owner, struct storage *store)
{
  int power;
  const resource_type *rtype;
  effect_data *edata = (effect_data *) a->data.v;
  char zText[32];

  READ_TOK(store, zText, sizeof(zText));
  rtype = rt_find(zText);

  READ_INT(store, &power);
  if (rtype == NULL || rtype->ptype == NULL || power <= 0) {
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
      data = (effect_data *) a->data.v;
      if (data->type == effect) {
        if (data->value + delta == 0) {
          a_remove(&u->attribs, a);
          return 0;
        } else {
          data->value += delta;
          return data->value;
        }
      }
      a = a->next;
    }

    a = a_add(&u->attribs, a_new(&at_effect));
    data = (effect_data *) a->data.v;
    data->type = effect;
    data->value = delta;
    return data->value;
  }
  log_error("change effect with delta==0 for unit %s\n", itoa36(u->no));
  return 0;
}
