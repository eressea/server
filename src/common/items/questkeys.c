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
#include "questkeys.h"

/* kernel includes */
#include <faction.h>
#include <item.h>
#include <message.h>
#include <plane.h>
#include <region.h>
#include <unit.h>
#include <border.h>

/* util includes */
#include <functions.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>

extern border *borders[];

static int
use_questkey(struct unit * u, const struct item_type * itype, int amount, const char * cmd)
{
	border *bo;
	int key, key1, key2;
	region *r1, *r2;
	int lock, k;
	message *m;
	unit *u2;

	if(u->region->x != 43 || u->region->y != -39) {
		ADDMSG(&u->faction->msgs, msg_error(u, cmd, "use_questkey_wrongregion", ""));
		return EUNUSABLE;
	}

	r1   = findregion(43,-39);
	r2   = findregion(44,-39);
	key1 =  region_hashkey(r1);
	key2 =  region_hashkey(r2);

	key = min(key2, key) % BMAXHASH;

	bo = borders[key];

	while (bo && !((bo->from==r1 && bo->to==r2) || (bo->from==r2 && bo->to==r1))) {
		if(bo->type == &bt_questportal) {
			break;
		}
		bo = bo->nexthash;
	}

	assert(bo != NULL);

	lock = (int)bo->data;
	if(itype == &it_questkey1) k = 1;
	if(itype == &it_questkey2) k = 2;

	if(lock & k) {
		m = msg_message("questportal_unlock","region unit key", u->region, u, k);
		lock = lock & ~k;
	} else {
		m = msg_message("questportal_lock","region unit key", u->region, u, k);
		lock = lock | k;
	}

	bo->data = (void *)lock;
		
	for(u2 = u->region->units; u2; u2=u2->next) {
		freset(u2->faction, FL_DH);
	}
	for(u2 = u->region->units; u2; u2=u2->next) {
		if(!fval(u2->faction, FL_DH)) {
			add_message(&u2->faction->msgs, m);
			fset(u2->faction, FL_DH);
		}
	}

	return 0;
}

static resource_type rt_questkey1 = {
	{ "questkey1", "questkey1_p" },
	{ "questkey1", "questkey1_p" },
	RTF_ITEM,
	&res_changeitem
};

static resource_type rt_questkey2 = {
	{ "questkey2", "questkey2_p" },
	{ "questkey2", "questkey2_p" },
	RTF_ITEM,
	&res_changeitem
};

item_type it_questkey1 = {
	&rt_questkey1, 	      /* resourcetype */
	ITF_NOTLOST, 1, 0,		/* flags, weight, capacity */
	NULL,								 	/* construction */
	&use_questkey,
	NULL,
	NULL
};

item_type it_questkey2 = {
	&rt_questkey2, 		  /* resourcetype */
	ITF_NOTLOST, 1, 0,	/* flags, weight, capacity */
	NULL,							 	/* construction */
	&use_questkey,
	NULL,
	NULL
};

void
register_questkeys(void)
{
	it_register(&it_questkey1);
	it_register(&it_questkey2);
	register_function((pf_generic)use_questkey, "usequestkey");
}

