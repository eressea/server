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
#include "plane.h"
#include "race.h"
#include "curse.h"
#include "region.h"
#include "karma.h"

#include <assert.h>
#include <stdlib.h>

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

skillmods * modhash[MAXRACES];

attrib *
make_skillmod(skill_t skill, unsigned int flags, int(*special)(const struct unit*, const struct region*, skill_t, int), double multiplier, int bonus)
{
	attrib * a = a_new(&at_skillmod);
	skillmod_data * smd = (skillmod_data*)a->data.v;

	smd->skill=skill;
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
att_modification(const unit *u, skill_t sk)
{
	curse *c;
	unit *mage;
	int result = 0;

	result += get_curseeffect(u->attribs, C_ALLSKILLS, 0);
	result += get_curseeffect(u->attribs, C_SKILL, (int)sk);

	/* TODO hier kann nicht mit get/iscursed gearbeitet werden, da nur der
	 * jeweils erste vom Typ C_GBDREAM zurückgegen wird, wir aber alle
	 * durchsuchen und aufaddieren müssen */
	if (is_cursed(u->region->attribs, C_GBDREAM, 0)){
		attrib *a;
		int bonus = 0, malus = 0;
		int mod;

		a = a_select(u->region->attribs, packids(C_GBDREAM, 0), cmp_oldcurse);
		while(a) {
			c = (curse*)a->data.v;
			mage = c->magician;
			mod = c->effect;
			/* wir suchen jeweils den größten Bonus und den größten Malus */
			if (mod > 0
					&& (mage == NULL
						|| allied(mage, u->faction, HELP_GUARD)))
			{
				if (mod > bonus ) bonus = mod;
				
			} else if (mod < 0
				&& (mage == NULL
					|| !allied(mage, u->faction, HELP_GUARD)))
			{
				if (mod < malus ) malus = mod;
			}
			a = a_select(a->next, packids(C_GBDREAM, 0), cmp_oldcurse);
		}
		result = result + bonus + malus;
	}

	return result;
}

void
item_modification(const unit *u, skill_t sk, int *val)
{
	/* Presseausweis: *2 Spionage, 0 Tarnung */
	if(sk == SK_SPY && get_item(u, I_PRESSCARD) >= u->number) {
		*val = *val * 2;
	}
	if(sk == SK_STEALTH && get_item(u, I_PRESSCARD) >= u->number) {
		*val = 0;
	}
}


signed char
skill_mod(race_t typ, skill_t sk, terrain_t t)
{
	signed char result = 0;

	result = race[typ].bonus[sk];

	switch (sk) {
	case SK_TACTICS:
		if (typ == RC_DWARF) {
			if (t == T_MOUNTAIN || t == T_GLACIER || t == T_ICEBERG_SLEEP) ++result;
		}
		break;
	}
	if (typ == RC_INSECT) {
		if (t == T_MOUNTAIN || t == T_GLACIER || t == T_ICEBERG_SLEEP || t == T_ICEBERG)
			--result;
		else if (t == T_DESERT || t == T_SWAMP)
			++result;
	}

	return result;
}

int modcount;

void
skill_done(void)
{
	race_t typ;
	for (typ=0;typ!=MAXRACES;++typ) {
		skillmods * mods = modhash[typ];
		if (mods) free(mods);
	}
}

void
skill_init(void)
{
	terrain_t t;
	skill_t sk;
	race_t typ;
	for (typ=0;typ!=MAXRACES;++typ) {
		skillmods * mods = modhash[typ];

		if (!mods) {
			++modcount;
			mods = (skillmods*)calloc(1, sizeof(skillmods));

			for (t=0;t!=MAXTERRAINS;++t) {
				for (sk=0;sk!=MAXSKILLS;++sk) {
					mods->mod[t].value[sk] = skill_mod(typ, sk, t);
				}
			}
			modhash[typ] = mods;
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
#ifdef SLOW_SKILLS
	/* that's just too many function calls, baby: */
	i = 0;
	while (level_days(i+1) <= days) ++i;
#else
	static int ldays[32];
	static boolean init = false;
	if (!init) {
		init = true;
		for (i=0;i!=32;++i) ldays[i] = level_days(i+1);
	}
	for (i=0;i!=32;++i) if (ldays[i]>days) return i;
#endif
	return i;
}

#ifdef OLD_FAMILIAR_MOD
extern int skillmod_familiar(const struct region *r, const struct unit *fam, skill_t sk);
#endif
char
eff_skill(const unit * u, skill_t sk, const region * r)
{
	int i, result = 0;

	if (u->number==0) return 0;
	if (r->planep && sk == SK_STEALTH && fval(r->planep, PFL_NOSTEALTH)) return 0;

	result = level(get_skill(u, sk) / u->number);
	if (result == 0) return 0;

	assert(r);
	result = (char)(result + modhash[u->race]->mod[rterrain(r)].value[sk]);

	if (u->race==RC_ELF && r_isforest(r)) switch (sk) {
	case SK_OBSERVATION:
		++result;
		break;
	case SK_STEALTH:
		if (r_isforest(r)) ++result;
		break;
	case SK_TACTICS:
		if (r_isforest(r)) result += 2;
		break;
	}

	result += att_modification(u, sk);
	item_modification(u, sk, &result);

	i = skillmod(u->attribs, u, r, sk, result, SMF_ALWAYS);
	if (i!=result)	{
		result = i;
	} else {
#ifdef OLD_TRIGGER
		unit * familiar = get_familiar(u);
		if (familiar!=NULL)
			result += skillmod_familiar(r, familiar, sk);
#endif
	}

	if (fspecial(u->faction, FS_TELEPATHY)) {
		switch(sk) {
		case SK_ALCHEMY:
		case SK_HERBALISM:
		case SK_MAGIC:
		case SK_SPY:
		case SK_STEALTH:
		case SK_OBSERVATION:
			break;
		default:
			result -= 2;
		}
	}


#ifdef HUNGER_REDUCES_SKILL
	if (fval(u, FL_HUNGER)) result = result/2;
#endif
	return (char)max(result, 0);
}

int
pure_skill(unit * u, skill_t sk, region * r)
{
	int  result = 0;
	unused(r);

	if (u->number==0) return 0;

	result = level(get_skill(u, sk) / u->number);

	return max(result, 0);
}

void
remove_zero_skills(void)
{
	region *r;
	unit *u;
	skill_t sk;
	
	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			for (sk = 0; sk != MAXSKILLS; sk++) {
				if(get_skill(u, sk) < u->number) {
					set_skill(u, sk, 0);
					
				}
			}
		}
	}
}
 
