/* vi: set ts=2:
 *
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
summon_igjarjuk(struct unit * u, const struct item_type * itype, int amount, const char * cmd)
{
	struct plane * p = rplane(u->region);
	unused(amount);
	unused(itype);
	if (p!=NULL) {
		ADDMSG(&u->faction->msgs, msg_error(u, cmd, "use_realworld_only", ""));
		return EUNUSABLE;
	} else {
		assert(!"not implemented");
		return EUNUSABLE;
	}
}

static resource_type rt_demonseye = {
	{ "ao_daemon", "ao_daemon_p" },
	{ "ao_daemon", "ao_daemon_p" },
	RTF_ITEM,
	&res_changeitem
};


boolean
give_igjarjuk(const struct unit * src, const struct unit * d, const struct item_type * itype, int n, const char * cmd)
{
	sprintf(buf, "Eine höhere Macht hindert %s daran, das Objekt zu übergeben. "
			"'ES IST DEINS, MEIN KIND. DEINS GANZ ALLEIN'.", unitname(src));
	mistake(src, cmd, buf, MSG_COMMERCE);
	return false;
}

item_type it_demonseye = {
	&rt_demonseye,           /* resourcetype */
	ITF_NOTLOST|ITF_CURSED, 0, 0,       /* flags, weight, capacity */
	NULL,                    /* construction */
	&summon_igjarjuk,
	NULL,
	&give_igjarjuk
};

void
register_demonseye(void)
{
	it_register(&it_demonseye);
	register_function((pf_generic)summon_igjarjuk, "useigjarjuk");
	register_function((pf_generic)give_igjarjuk, "giveigjarjuk");
}
