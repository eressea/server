/* vi: set ts=2:
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "demonseye.h"

/* kernel includes */
#include <faction.h>
#include <item.h>
#include <message.h>
#include <plane.h>
#include <region.h>
#include <unit.h>

/* util includes */
#include <functions.h>

/* libc includes */
#include <assert.h>

static int
summon_igjarjuk(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
	struct plane * p = rplane(u->region);
	unused(amount);
	unused(itype);
	if (p!=NULL) {
		ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "use_realworld_only", ""));
		return EUNUSABLE;
	} else {
		assert(!"not implemented");
		return EUNUSABLE;
	}
}

static boolean
give_igjarjuk(const struct unit * src, const struct unit * d, const struct item_type * itype, int n, struct order * ord)
{
  ADDMSG(&src->faction->msgs, msg_feedback(src, ord, "error_giveeye", ""));
	return false;
}

void
register_demonseye(void)
{
	register_function((pf_generic)summon_igjarjuk, "useigjarjuk");
	register_function((pf_generic)give_igjarjuk, "giveigjarjuk");
}
