/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>

#if GROWING_TREES
#include "seed.h"

#include <build.h>
#include <region.h>

/* kernel includes */
#include <item.h>

/* libc includes */
#include <assert.h>

resource_type rt_seed = {
	{ "seed", "seed_p" },
	{ "seed", "seed_p" },
	RTF_ITEM|RTF_LIMITED,
	&res_changeitem
};

static construction con_seed = {
	SK_HERBALISM, 3,  /* skill, minskill */
	-1, 1             /* maxsize, reqsize [,materials] */
};

item_type it_seed = {
	&rt_seed,    /* resourcetype */
	0, 10, 0,    /* flags, weight, capacity */
	&con_seed,   /* construction */
	NULL,        /* use */
	NULL         /* give */
};

static void
produce_seeds(region * r, const resource_type * rtype, int norders)
{
	assert(rtype==&rt_seed && r->land && r->land->trees[0] >= norders);
	r->land->trees[0] -= norders;
}

static int
limit_seeds(const region * r, const resource_type * rtype)
{
	assert(rtype==&rt_seed);
	if(fval(r, RF_MALLORN)) return 0;
	return r->land?r->land->trees[0]:0;
}

void
init_seed(void)
{
	attrib * a;

	it_register(&it_seed);
	it_seed.rtype->flags |= RTF_LIMITED;
	it_seed.rtype->itype->flags |= ITF_NOBUILDBESIEGED;
	it_seed.rtype->flags |= RTF_POOLED;

	a = a_add(&it_seed.rtype->attribs, a_new(&at_resourcelimit));
	{
		resource_limit * rdata = (resource_limit*)a->data.v;
		rdata->limit = limit_seeds;
		rdata->use = produce_seeds;
	}
}

/* mallorn */

resource_type rt_mallornseed = {
	{ "mallornseed", "mallornseed_p" },
	{ "mallornseed", "mallornseed_p" },
	RTF_ITEM|RTF_LIMITED,
	&res_changeitem
};

static construction con_mallornseed = {
	SK_HERBALISM, 4,  /* skill, minskill */
	-1, 1             /* maxsize, reqsize [,materials] */
};

item_type it_mallornseed = {
	&rt_mallornseed,    /* resourcetype */
	0, 10, 0,    /* flags, weight, capacity */
	&con_mallornseed,   /* construction */
	NULL,        /* use */
	NULL         /* give */
};

static void
produce_mallornseeds(region * r, const resource_type * rtype, int norders)
{
	assert(rtype==&rt_mallornseed && r->land && r->land->trees[0] >= norders);
	assert(fval(r, RF_MALLORN));
	r->land->trees[0] -= norders;
}

static int
limit_mallornseeds(const region * r, const resource_type * rtype)
{
	assert(rtype==&rt_mallornseed);
	if(! fval(r, RF_MALLORN)) {
		return 0;
	}
	return r->land?r->land->trees[0]:0;
}

void
init_mallornseed(void)
{
	attrib * a;

	it_register(&it_mallornseed);
	it_mallornseed.rtype->flags |= RTF_LIMITED;
	it_mallornseed.rtype->itype->flags |= ITF_NOBUILDBESIEGED;
	it_mallornseed.rtype->flags |= RTF_POOLED;

	a = a_add(&it_mallornseed.rtype->attribs, a_new(&at_resourcelimit));
	{
		resource_limit * rdata = (resource_limit*)a->data.v;
		rdata->limit = limit_mallornseeds;
		rdata->use = produce_mallornseeds;
	}
}

#endif
