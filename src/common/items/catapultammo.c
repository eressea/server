/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>

#include "catapultammo.h"

#include <build.h>
#include <region.h>

/* kernel includes */
#include <item.h>

/* libc includes */
#include <assert.h>

static requirement mat_catapultammo[] = {
	  {I_STONE, 1},
		{0, 0}
};

resource_type rt_catapultammo = {
	{ "catapultammo", "catapultammo_p" },
	{ "catapultammo", "catapultammo_p" },
	RTF_ITEM|RTF_POOLED,
	&res_changeitem
};

static construction con_catapultammo = {
	SK_QUARRYING, 3,  /* skill, minskill */
	-1, 1, mat_catapultammo /* maxsize, reqsize [,materials] */
};

item_type it_catapultammo = {
	&rt_catapultammo,    /* resourcetype */
	0, 1000, 0,   			 /* flags, weight, capacity */
	&con_catapultammo,   /* construction */
	NULL,                /* use */
	NULL                 /* give */
};

void
register_catapultammo(void)
{
	it_register(&it_catapultammo);
}

