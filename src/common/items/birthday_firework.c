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
#include "birthday_firework.h"

/* kernel includes */
#include <item.h>
#include <build.h>
#include <region.h>
#include <teleport.h>
#include <unit.h>
#include <message.h>

/* util includes */
#include <functions.h>
#include "../util/message.h"

/* libc includes */
#include <assert.h>

const int FIREWORK_RANGE=10;


static int
use_birthday_firework(struct unit * u, const struct item_type * itype, const char *cm)
{
	regionlist *rlist = all_in_range(u->region, FIREWORK_RANGE);
	regionlist *rl;
	message *m;
	char *name;

	name = getstrtoken();

	/* "Zur Feier der Geburtstags von %name wird in %region ein großes
	 * Feuerwerk abgebrannt, welches noch hier zu bewundern ist. Kaskaden
	 * bunter Sterne, leuchtende Wasserfälle aus Licht und strahlende
	 * Feuerdrachen erhellen den Himmel." */

	if(name && *name) {
		m = new_message(u->faction, "birthday_firework%u:unit%r:region%s:name",
			u, u->region, strdup(name));
		add_message(&u->region->msgs, new_message(u->faction, "birthday_firework_local%u:unit%s:name",u, strdup(name)));
	} else {
		m = new_message(u->faction, "birthday_firework_noname%u:unit%r:region",
			u, u->region);
		add_message(&u->region->msgs, new_message(u->faction, "birthday_firework_noname_local%u:unit%s:name",u));
	}

	for(rl = rlist; rl; rl=rl->next) if(rl->region != u->region) {
		add_message(&rl->region->msgs, m);
	}

	msg_release(m);
	free_regionlist(rlist);

	res_changeitem(u, itype->rtype, -1);

	return -1;
}

resource_type rt_birthday_firework = {
	{ "birthday_firework", "birthday_firework_p" },
	{ "birthday_firework", "birthday_firework_p" },
	RTF_ITEM,
	&res_changeitem
};

item_type it_birthday_firework = {
	&rt_birthday_firework,   	/* resourcetype */
	0, 50, 0,			/* flags, weight, capacity */
	NULL,					/* construction */
	&use_birthday_firework,	/* use */
	NULL						/* give */
};

void
register_birthday_firework(void)
{
	it_register(&it_birthday_firework);
	it_birthday_firework.rtype->flags |= RTF_POOLED;
	register_function((pf_generic)use_birthday_firework, "usefirework");
}

resource_type rt_lebkuchenherz = {
	{ "lebkuchenherz", "lebkuchenherz_p" },
	{ "lebkuchenherz", "lebkuchenherz_p" },
	RTF_ITEM,
	&res_changeitem
};

item_type it_lebkuchenherz = {
	&rt_lebkuchenherz,   	/* resourcetype */
	0, 0, 0,			/* flags, weight, capacity */
	NULL,					/* construction */
	NULL,					/* use */
	NULL					/* give */
};

void
register_lebkuchenherz(void)
{
	it_register(&it_lebkuchenherz);
	it_lebkuchenherz.rtype->flags |= RTF_POOLED;
}

