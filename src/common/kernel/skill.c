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
#include "skill.h"

#include "unit.h"
#include "item.h"
#include "magic.h"
#include "race.h"
#include "curse.h"
#include "region.h"
#include "karma.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Umlaute hier drin, weil die in den Report kommen */
static const char *skillnames[MAXSKILLS] =
{
	"sk_alchemy",
	"sk_crossbow",
	"sk_mining",
	"sk_bow",
	"sk_building",
	"sk_trade",
	"sk_forestry",
	"sk_catapult",
	"sk_herbalism",
	"sk_magic",
	"sk_training",
	"sk_riding",
	"sk_armorer",
	"sk_shipcraft",
	"sk_melee",
	"sk_sailing",
	"sk_polearm",
	"sk_espionage",
	"sk_quarrying",
	"sk_roadwork",
	"sk_tactics",
	"sk_stealth",
	"sk_entertainment",
	"sk_weaponsmithing",
	"sk_cartmaking",
	"sk_perception",
	"sk_taxation",
	"sk_stamina",
	"sk_unarmed"
};

const char * 
skillname(skill_t sk, const struct locale * lang)
{
	return locale_string(lang, skillnames[sk]);
}

skill_t
sk_find(const char * name)
{
	skill_t i;
	if (name==NULL) return NOSKILL;
	for (i=0;i!=MAXSKILLS;++i) {
		if (strcmp(name, skillnames[i])==0) return i;
	}
	return NOSKILL;
}

/** skillmod attribut **/
static void
init_skillmod(attrib * a) {
	a->data.v = calloc(sizeof(skillmod_data), 1);
}

static void
finalize_skillmod(attrib * a)
{
	free(a->data.v);
}

attrib_type at_skillmod = {
	"skillmod",
	init_skillmod,
	finalize_skillmod,
	NULL,
	NULL, /* can't write function pointers */
	NULL, /* can't read function pointers */
	ATF_PRESERVE
};

attrib *
make_skillmod(skill_t sk, unsigned int flags, int(*special)(const struct unit*, const struct region*, skill_t, int), double multiplier, int bonus)
{
	attrib * a = a_new(&at_skillmod);
	skillmod_data * smd = (skillmod_data*)a->data.v;

	smd->skill=sk;
	smd->special=special;
	smd->bonus=bonus;
	smd->multiplier=multiplier;
	smd->flags=flags;

	return a;
}

int
skillmod(const attrib * a, const unit * u, const region * r, skill_t sk, int value, int flags)
{
	for (a = a_find((attrib*)a, &at_skillmod); a; a=a->nexttype) {
		skillmod_data * smd = (skillmod_data *)a->data.v;
		if (smd->skill!=NOSKILL && smd->skill!=sk) continue;
		if (flags!=SMF_ALWAYS && (smd->flags & flags) == 0) continue;
		if (smd->special) {
			value = smd->special(u, r, sk, value);
			if (value<0) return value; /* pass errors back to caller */
		}
		if (smd->multiplier) value = (int)(value*smd->multiplier);
		value += smd->bonus;
	}
	return value;
}

int
skill_mod(const race * rc, skill_t sk, terrain_t t)
{
	int result = 0;

	result = rc->bonus[sk];

	switch (sk) {
	case SK_TACTICS:
		if (rc == new_race[RC_DWARF]) {
			if (t == T_MOUNTAIN || t == T_GLACIER || t == T_ICEBERG_SLEEP) ++result;
		}
		break;
	}
	if (rc == new_race[RC_INSECT]) {
		if (t == T_MOUNTAIN || t == T_GLACIER || t == T_ICEBERG_SLEEP || t == T_ICEBERG)
			--result;
		else if (t == T_DESERT || t == T_SWAMP)
			++result;
	}
	
	return result;
}

#define RCMODMAXHASH 31
static struct skillmods {
	struct skillmods * next;
	const struct race * race;
	struct modifiers {
		int value[MAXSKILLS];
	} mod[MAXTERRAINS];
} * modhash[RCMODMAXHASH];

static struct skillmods *
init_skills(const race * rc)
{
	terrain_t t;
	struct skillmods *mods = (struct skillmods*)calloc(1, sizeof(struct skillmods));
	mods->race = rc;

	for (t=0;t!=MAXTERRAINS;++t) {
		skill_t sk;
		for (sk=0;sk!=MAXSKILLS;++sk) {
			mods->mod[t].value[sk] = skill_mod(rc, sk, t);
		}
	}
	return mods;
}

int
rc_skillmod(const struct race * rc, const region *r, skill_t sk)
{
	int mods;
	unsigned int index = ((unsigned int)rc) % RCMODMAXHASH;
	struct skillmods **imods = &modhash[index];
	while (*imods && (*imods)->race!=rc) imods = &(*imods)->next;
	if (*imods==NULL) {
		*imods = init_skills(rc);
	}
	mods = (*imods)->mod[rterrain(r)].value[sk];

	if (rc == new_race[RC_ELF] && r_isforest(r)) switch (sk) {
	case SK_OBSERVATION:
		++mods;
		break;
	case SK_STEALTH:
		if (r_isforest(r)) ++mods;
		break;
	case SK_TACTICS:
		if (r_isforest(r)) mods += 2;
		break;
	}

	return mods;
}

void
skill_init(void)
{
}

void
skill_done(void)
{
	int i;
	for (i = 0;i!=RCMODMAXHASH;++i) {
		while (modhash[i]) {
			struct skillmods * mods = modhash[i];
			modhash[i] = mods->next;
			free(mods);
		}
	}
}

int
level_days(int level)
{
	return 30 * ((level+1) * level / 2);
}

int
level(int days)
{
	int i;
	static int ldays[64];
	static boolean init = false;
	if (!init) {
		init = true;
		for (i=0;i!=64;++i) ldays[i] = level_days(i+1);
	}
	for (i=0;i!=64;++i) if (ldays[i]>days) return i;
	return i;
}

#if !SKILLPOINTS
void 
reduce_skill(skill * sv, int change)
{
	if (sv->learning>=change) {
		/* just forget a few weeks */
		sv->learning = (unsigned char)(sv->learning-change);
	} else {
		int weeks;
		change -= sv->learning;
		while (change>=sv->level && sv->level > 0) {
			change -= sv->level;
			--sv->level;
		}
		if (change && sv->level > 0 && change<sv->level) {
			/* this is not an exact science... it would be better to set
			 * sv->learning so that the average time to rise was = change.
			 */
			--sv->level;
			weeks = (sv->level-change)*2;
		} else {
			weeks = 0;
		}
		sv->learning = (unsigned char)weeks;
	}
	assert(sv->level>=0 && sv->learning>=0 && sv->learning<=sv->level*2);
}

int 
skill_compare(const skill * sk, const skill * sc)
{
	if (sk->level > sc->level) return 1;
	if (sk->level < sc->level) return -1;
	if (sk->learning > sc->learning) return 1;
	if (sk->learning < sc->learning) return -1;
	return 0;
}
#endif
