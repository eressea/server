/* vi: set ts=2:
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "xerewards.h"

/* kernel includes */
#include <item.h>
#include <region.h>
#include <faction.h>
#include <unit.h>
#include <skill.h>
#include <curse.h>
#include <message.h>
#include <magic.h>

/* util includes */
#include <functions.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int
use_skillpotion(struct unit * u, const struct item_type * itype, const char *cm)
{
	skill * sv = u->skills;
	while (sv!=u->skills+u->skill_size) {
		int i;
		for (i=0;i!=3;++i) learn_skill(u, sv->id, 1.0);
		++sv;
	}
	add_message(&u->faction->msgs, new_message(u->faction,
		"skillpotion_use%u:unit", u));

	res_changeitem(u, itype->rtype, -1);
	return -1;
}

static resource_type rt_skillpotion = {
	{ "skillpotion", "skillpotion_p" },
	{ "skillpotion", "skillpotion_p" },
	RTF_ITEM,
	&res_changeitem
};

item_type it_skillpotion = {
	&rt_skillpotion,        /* resourcetype */
	0, 0, 0,		/* flags, weight, capacity */
	NULL,                   /* construction */
	&use_skillpotion,
	NULL
};


static int
use_manacrystal(struct unit * u, const struct item_type * itype, const char *cm)
{
	int     sp;

	if(!is_mage(u)) {
		cmistake(u, u->thisorder, 295, MSG_EVENT);
		return -1;
	}

	sp = max(25, max_spellpoints(u->region, u)/2);

	change_spellpoints(u, sp);

	add_message(&u->faction->msgs, new_message(u->faction,
		"manacrystal_use%u:unit%i:aura", u, sp));

	res_changeitem(u, itype->rtype, -1);
	return -1;
}

static resource_type rt_manacrystal = {
	{ "manacrystal", "manacrystal_p" },
	{ "manacrystal", "manacrystal_p" },
	RTF_ITEM,
	&res_changeitem
};

item_type it_manacrystal = {
	&rt_manacrystal,        /* resourcetype */
	0, 0, 0,		/* flags, weight, capacity */
	NULL,                   /* construction */
	&use_manacrystal,
	NULL
};


void
register_xerewards(void)
{
	it_register(&it_skillpotion);
	it_register(&it_manacrystal);
	register_function((pf_generic)use_skillpotion, "useskillpotion");
	register_function((pf_generic)use_manacrystal, "usemanacrystal");
}

