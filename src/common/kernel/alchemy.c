/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#include "alchemy.h"

#include "item.h"
#include "faction.h"
#include "message.h"
#include "build.h"
#include "magic.h"
#include "region.h"
#include "pool.h"
#include "race.h"
#include "unit.h"
#include "skill.h"
#include "move.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/log.h>
#include <util/rand.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------------------------------- */

void
herbsearch(region * r, unit * u, int max)
{
  int herbsfound;
  const item_type * whichherb;

  if (eff_skill(u, SK_HERBALISM, r) == 0) {
    cmistake(u, u->thisorder, 59, MSG_PRODUCE);
    return;
  }

  if(is_guarded(r, u, GUARD_PRODUCE)) {
    cmistake(u, u->thisorder, 70, MSG_EVENT);
    return;
  }

  whichherb = rherbtype(r);
  if (whichherb == NULL) {
    cmistake(u, u->thisorder, 108, MSG_PRODUCE);
    return;
  }

  if (max) max = min(max, rherbs(r));
  else max = rherbs(r);
  herbsfound = ntimespprob(eff_skill(u, SK_HERBALISM, r) * u->number,
    (double)rherbs(r)/100.0F, -0.01F);
  herbsfound = min(herbsfound, max);
  rsetherbs(r, rherbs(r)-herbsfound);

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

int
use_potion(unit * u, const item_type * itype, int amount, struct order *ord)
{
  const potion_type * ptype = resource2potion(itype->rtype);
  const potion_type * use = ugetpotionuse(u);
  assert(ptype!=NULL);
  
  if (use != NULL && use!=ptype) {
    ADDMSG(&u->faction->msgs,
           msg_message("errusingpotion", "unit using command",
                       u, use->itype->rtype, ord));
    return ECUSTOM;
  }
  
  if (ptype==oldpotiontype[P_LIFE]) {
    region * r = u->region;
    int holz = 0;
    
    /* für die Aufforstung von Mallornwäldern braucht man Mallorn */
    if (fval(r, RF_MALLORN)) {
      holz = use_pooled(u, rt_find("mallorn"), 
                        GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, 10*amount);
    } else {
      holz = use_pooled(u, rt_find("log"), 
                        GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, 10*amount);
    }
    if (r->land==0) holz=0;
    if (holz<10*amount) {
      int x = holz/10;
      if (holz%10) ++x;
      if (x<amount) amount = x;
    }
    rsettrees(r, 1, rtrees(r, 1) + holz);
    ADDMSG(&u->faction->msgs, msg_message("growtree_effect", 
      "mage amount", u, holz));
  } else if (ptype==oldpotiontype[P_HEAL]) {
    return EUNUSABLE;
  } else if (ptype==oldpotiontype[P_HEILWASSER]) {
    u->hp = min(unit_max_hp(u) * u->number, u->hp + 400 * amount);
  } else if (ptype==oldpotiontype[P_PEOPLE]) {
    region * r = u->region;
    attrib * a = (attrib*)a_find(r->attribs, &at_peasantluck);
    if (!a) a = a_add(&r->attribs, a_new(&at_peasantluck));
    a->data.i+=amount;
  } else if (ptype==oldpotiontype[P_HORSE]) {
    region * r = u->region;
    attrib * a = (attrib*)a_find(r->attribs, &at_horseluck);
    if (!a) a = a_add(&r->attribs, a_new(&at_horseluck));
    a->data.i+=amount;
  } else if (ptype==oldpotiontype[P_WAHRHEIT]) {
    fset(u, UFL_DISBELIEVES);
    amount=1;
  } else if (ptype==oldpotiontype[P_MACHT]) {
    /* Verfünffacht die HP von max. 10 Personen in der Einheit */
    u->hp += min(u->number, 10*amount) * unit_max_hp(u) * 4;
  } else {
    change_effect(u, ptype, 10*amount);
  }
  use_pooled(u, ptype->itype->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, amount);
  usetpotionuse(u, ptype);
  
  ADDMSG(&u->faction->msgs, msg_message("usepotion",
    "unit potion", u, ptype->itype->rtype));
  return 0;
}

/*****************/
/*   at_effect   */
/*****************/

static void
a_initeffect(attrib *a)
{
	a->data.v = calloc(sizeof(effect_data), 1);
}

static void
a_finalizeeffect(attrib * a)
{
	free(a->data.v);
}

static void
a_writeeffect(const attrib *a, FILE *f)
{
	effect_data * edata = (effect_data*)a->data.v;
	fprintf(f, "%s %d ", resourcename(edata->type->itype->rtype, 0), edata->value);
}

static int
a_readeffect(attrib *a, FILE *f)
{
  int power, result;
  const item_type * itype;
  effect_data * edata = (effect_data*)a->data.v;
  char zText[32];

  result = fscanf(f, "%s %d", zText, &power);
  if (result<0) return result;
  itype = it_find(zText);
  if (itype==NULL || itype->rtype==NULL || itype->rtype->ptype==NULL || power<=0) {
    return AT_READ_FAIL;
  }
  edata->type = itype->rtype->ptype;
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

int
get_effect(const unit * u, const potion_type * effect)
{
	const attrib * a;
	for (a=a_find(u->attribs, &at_effect); a!=NULL && a->type==&at_effect; a=a->next) {
		const effect_data * data = (const effect_data *)a->data.v;
		if (data->type==effect) return data->value;
	}
	return 0;
}

int
change_effect (unit * u, const potion_type * effect, int delta)
{
  if (delta!=0) {
    attrib * a = a_find(u->attribs, &at_effect);
    effect_data * data = NULL;

    while (a && a->type==&at_effect) {
      data = (effect_data *)a->data.v;
      if (data->type==effect) {
        if (data->value+delta==0) {
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
    data = (effect_data*)a->data.v;
    data->type = effect;
    data->value = delta;
    return data->value;
  }
  log_error(("change effect with delta==0 for unit %s\n", itoa36(u->no)));
  return 0;
}
