/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
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

/* util includes */
#include <rand.h>

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
	const herb_type * whichherb;
	if (eff_skill(u, SK_HERBALISM, r) == 0) {
		cmistake(u, u->thisorder, 59, MSG_PRODUCE);
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
			(double)rherbs(r)/100.0L, -0.01L);
	herbsfound = min(herbsfound, max);
	rsetherbs(r, rherbs(r)-herbsfound);

	if (herbsfound) {
		change_skill(u, SK_HERBALISM, PRODUCEEXP * u->number);
		i_change(&u->items, whichherb->itype, herbsfound);
		add_message(&u->faction->msgs, new_message(u->faction,
			"herbfound%u:unit%r:region%i:amount%X:herb", u, r, herbsfound,
			herb2resource(whichherb)));
	} else {
		add_message(&u->faction->msgs, new_message(u->faction,
			"researchherb_none%u:unit%r:region", u, u->region));
	}
}

attrib_type at_bauernblut = {
	"bauernblut", NULL, NULL, NULL, NULL, NULL
};

int
use_potion(unit * u, const item_type * itype, const char * cmd)
{
	const potion_type * ptype = resource2potion(itype->rtype);
	const potion_type * use = ugetpotionuse(u);
	assert(ptype!=NULL);

	if (use != NULL && use!=ptype) {
		add_message(&u->faction->msgs,
					new_message(u->faction,
								"errusingpotion%u:unit%X:using%s:command",
								u, use->itype->rtype, cmd));
		return ECUSTOM;
	}

	if (ptype->use) {
		int nRetval = ptype->use(u, ptype, cmd);
		if (nRetval) return nRetval;
	} else if (ptype==oldpotiontype[P_LIFE]) {
		region * r = u->region;
		int holz = 0;

		/* für die Aufforstung von Mallornwäldern braucht man Mallorn */
		if (fval(r, RF_MALLORN)) {
			holz = new_use_pooled(u, oldresourcetype[R_MALLORN], 
					GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, 10);
		} else {
			holz = new_use_pooled(u, oldresourcetype[R_WOOD], 
					GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, 10);
		}
#ifdef GROWING_TREES
		rsettrees(r, 1, rtrees(r, 1) + holz);
#else
		rsettrees(r, rtrees(r) + holz);
#endif
		add_message(&u->faction->msgs, new_message(u->faction,
			"growtree_effect%u:mage%i:amount", u, holz));
	} else if (ptype==oldpotiontype[P_HEAL]) {
		return EUNUSABLE;
	} else if (ptype==oldpotiontype[P_HEILWASSER]) {
		u->hp = min(unit_max_hp(u) * u->number, u->hp + 400);
	} else if (ptype==oldpotiontype[P_PEOPLE]) {
		region * r = u->region;
		attrib * a = (attrib*)a_find(r->attribs, &at_peasantluck);
		if (!a) a = a_add(&r->attribs, a_new(&at_peasantluck));
		++a->data.i;
	} else if (ptype==oldpotiontype[P_HORSE]) {
		region * r = u->region;
		attrib * a = (attrib*)a_find(r->attribs, &at_horseluck);
		if (!a) a = a_add(&r->attribs, a_new(&at_horseluck));
		++a->data.i;
	} else if (ptype==oldpotiontype[P_WAHRHEIT]) {
		fset(u, FL_DISBELIEVES);
	} else if (ptype==oldpotiontype[P_MACHT]) {
		/* Verfünffacht die HP von max. 10 Personen in der Einheit */
		u->hp += min(u->number, 10) * unit_max_hp(u) * 4;
	} else {
		change_effect(u, ptype, 10);
	}
	new_use_pooled(u, ptype->itype->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, 1);
	usetpotionuse(u, ptype);

	add_message(&u->faction->msgs, new_message(u->faction,
		"usepotion%u:unit%X:potion", u, ptype->itype->rtype));
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
	int power;
	const potion_type * ptype;
	effect_data * edata = (effect_data*)a->data.v;
	if (global.data_version < ITEMTYPE_VERSION) {
		union {
			int i;
			short sa[2];
		} data;
		fscanf(f, "%d", &data.i);
		ptype = oldpotiontype[data.sa[0]];
		power = data.sa[1];
	} else {
		char zText[32];
		fscanf(f, "%s %d", zText, &power);
		ptype = pt_find(zText);
	}
	if (ptype==NULL || power==0) return 0;
	edata->type = ptype;
	edata->value = power;
	return 1;
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
	for (a=a_find(u->attribs, &at_effect); a!=NULL; a=a->nexttype) {
		const effect_data * data = (const effect_data *)a->data.v;
		if (data->type==effect) return data->value;
	}
	return 0;
}

int
set_effect (unit * u, const potion_type * effect, int value)
{
	attrib ** ap = &u->attribs, * a;
	effect_data * data = NULL;
	while (*ap && (*ap)->type!=&at_effect) ap=&(*ap)->next;
	a = *ap;
	while (a) {
		data = (effect_data *)a->data.v;
		if (data->type==effect) break;
		a=a->nexttype;
	}
	if (!a && value) {
		attrib * an = a_add(ap, a_new(&at_effect));
		data = (effect_data*)an->data.v;
		data->type = effect;
		data->value = value;
	} else if (a && !value) {
		a_remove(ap, a);
	} else if (a && value) {
		data->value = value;
	}
	return value;
}

int
change_effect (unit * u, const potion_type * effect, int delta)
{
	attrib ** ap = &u->attribs, * a;
	effect_data * data = NULL;

	assert(delta!=0);
	while (*ap && (*ap)->type!=&at_effect) ap=&(*ap)->next;
	a = *ap;
	while (a) {
		data = (effect_data *)a->data.v;
		if (data->type==effect) break;
		a=a->nexttype;
	}
	if (a!=NULL && data->value+delta==0) {
		a_remove(ap, a);
		return 0;
	} else if (a!=NULL) {
		data->value += delta;
	} else {
		attrib * an = a_add(ap, a_new(&at_effect));
		data = (effect_data*)an->data.v;
		data->type = effect;
		data->value = delta;
	}
	return data->value;
}
