/* vi: set ts=2:
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
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
use_skillpotion(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
  /* the problem with making this a lua function is that there's no way
   * to get the list of skills for a unit. and with the way skills are 
   * currently saved, it doesn't look likely (can't make eressea::list 
   * from them)
   */
	int i;
	for (i=0;i!=amount;++i) {
		skill * sv = u->skills;
		while (sv!=u->skills+u->skill_size) {
			int i;
			for (i=0;i!=3;++i) learn_skill(u, sv->id, 1.0);
			++sv;
		}
	}
	add_message(&u->faction->msgs, new_message(u->faction,
		"skillpotion_use%u:unit", u));

	res_changeitem(u, itype->rtype, -amount);
	return 0;
}

static int
use_manacrystal(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
	int i, sp = 0;

	if(!is_mage(u)) {
		cmistake(u, u->thisorder, 295, MSG_EVENT);
		return -1;
	}

	for (i=0;i!=amount;++i) {
		sp += max(25, max_spellpoints(u->region, u)/2);
		change_spellpoints(u, sp);
	}

	add_message(&u->faction->msgs, new_message(u->faction,
		"manacrystal_use%u:unit%i:aura", u, sp));

	res_changeitem(u, itype->rtype, -amount);
	return 0;
}

void
register_xerewards(void)
{
	register_function((pf_generic)use_skillpotion, "useskillpotion");
	register_function((pf_generic)use_manacrystal, "usemanacrystal");
}

