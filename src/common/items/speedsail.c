/* vi: set ts=2:
 *
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
#include "speedsail.h"

/* kernel includes */
#include <faction.h>
#include <item.h>
#include <message.h>
#include <movement.h>
#include <plane.h>
#include <region.h>
#include <ship.h>
#include <unit.h>

/* util includes */
#include <functions.h>

/* libc includes */
#include <assert.h>

static int
use_speedsail(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
	struct plane * p = rplane(u->region);
	unused(amount);
	unused(itype);
	if (p!=NULL) {
		ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "use_realworld_only", ""));
		return EUNUSABLE;
	} else {
    if (u->ship) {
      attrib * a = a_find(u->ship->attribs, &at_speedup);
      if (a!=NULL) {
        a = a_add(&u->ship->attribs, a_new(&at_speedup));
        a->data.sa[0] = 50; /* speed */
        a->data.sa[1] = 50; /* decay */
        ADDMSG(&u->faction->msgs, msg_message("use_speedsail", "unit", u));
        return 0;
      } else {
        cmistake(u, ord, 211, MSG_EVENT);
      }
    } else {
      cmistake(u, ord, 144, MSG_EVENT);
    }
		return EUNUSABLE;
	}
}

static resource_type rt_speedsail = {
	{ "speedsail", "speedsail_p" },
	{ "speedsail", "speedsail_p" },
	RTF_ITEM,
	&res_changeitem
};


item_type it_speedsail = {
	&rt_speedsail,           /* resourcetype */
	0, 0, 0,       /* flags, weight, capacity */
	NULL,                    /* construction */
	&use_speedsail,
	NULL,
	NULL
};

void
register_speedsail(void)
{
	it_register(&it_speedsail);
}
