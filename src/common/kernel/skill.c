/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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

#include "curse.h"
#include "item.h"
#include "magic.h"
#include "race.h"
#include "region.h"
#include "terrain.h"
#include "terrainid.h"
#include "unit.h"
#include "karma.h"

#include <util/attrib.h>
#include <util/goodies.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *skillnames[MAXSKILLS] =
{
	"alchemy",
	"crossbow",
	"mining",
	"bow",
	"building",
	"trade",
	"forestry",
	"catapult",
	"herbalism",
	"magic",
	"training",
	"riding",
	"armorer",
	"shipcraft",
	"melee",
	"sailing",
	"polearm",
	"espionage",
	"quarrying",
	"roadwork",
	"tactics",
	"stealth",
	"entertainment",
	"weaponsmithing",
	"cartmaking",
	"perception",
	"taxation",
	"stamina",
	"unarmed"
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
  if (strncmp(name, "sk_", 3)==0) name+=3;
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
make_skillmod(skill_t sk, unsigned int flags, skillmod_fun special, double multiplier, int bonus)
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
skill_mod(const race * rc, skill_t sk, const struct terrain_type * terrain)
{
	int result = 0;

	result = rc->bonus[sk];

  if (rc == new_race[RC_DWARF]) {
    if (sk==SK_TACTICS) {
      terrain_t t = oldterrain(terrain);
      if (t == T_MOUNTAIN || t == T_GLACIER || t == T_ICEBERG_SLEEP) ++result;
    }
  }
	else if (rc == new_race[RC_INSECT]) {
    terrain_t t = oldterrain(terrain);
		if (t == T_MOUNTAIN || t == T_GLACIER || t == T_ICEBERG_SLEEP || t == T_ICEBERG)
			--result;
		else if (t == T_DESERT || t == T_SWAMP)
			++result;
	}
	
	return result;
}

#define RCMODMAXHASH 31
#ifdef FASTER_SKILLMOD
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
			mods->mod[t].value[sk] = skill_mod(rc, sk, newterrain(t));
		}
	}
	return mods;
}
#endif

int
rc_skillmod(const struct race * rc, const region *r, skill_t sk)
{
	int mods;
#ifdef FASTER_SKILLMOD
  unsigned int index = hashstring(rc->_name[0]) % RCMODMAXHASH;
	struct skillmods **imods = &modhash[index];
	while (*imods && (*imods)->race!=rc) imods = &(*imods)->next;
	if (*imods==NULL) {
		*imods = init_skills(rc);
	}
	mods = (*imods)->mod[rterrain(r)].value[sk];
#else
  mods = skill_mod(rc, sk, r->terrain);
#endif
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
#ifdef FASTER_SKILLMOD
	int i;
	for (i = 0;i!=RCMODMAXHASH;++i) {
		while (modhash[i]) {
			struct skillmods * mods = modhash[i];
			modhash[i] = mods->next;
			free(mods);
		}
	}
#endif
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

void
sk_set(skill * sv, int level)
{
	assert(level!=0);
	sv->weeks = (unsigned char)skill_weeks(level);
	sv->level = (unsigned char)level;
}

int
skill_weeks(int level)
/* how many weeks must i study to get from level to level+1 */
{
	int coins = 2*level;
	int heads = 1;
	while (coins--) {
		heads += rand() % 2;
	}
	return heads;
}

void 
reduce_skill(unit * u, skill * sv, unsigned int weeks)
{
	sv->weeks+=weeks;
	while (sv->level>0 && sv->level*2+1<sv->weeks) {
		sv->weeks -= sv->level;
		--sv->level;
	}
	if (sv->level==0) {
		/* reroll */
		sv->weeks = (unsigned char)skill_weeks(sv->level);
	}
}


int 
skill_compare(const skill * sk, const skill * sc)
{
	if (sk->level > sc->level) return 1;
	if (sk->level < sc->level) return -1;
	if (sk->weeks < sc->weeks) return 1;
	if (sk->weeks > sc->weeks) return -1;
	return 0;
}
