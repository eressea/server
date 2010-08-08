/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>

#include "seed.h"

/* kernel includes */
#include <kernel/build.h>
#include <kernel/config.h>
#include <kernel/item.h>
#include <kernel/region.h>

/* util includes */
#include <util/attrib.h>
#include <util/functions.h>

/* libc includes */
#include <assert.h>

resource_type * rt_seed = 0;
resource_type * rt_mallornseed = 0;

static void
produce_seeds(region * r, const resource_type * rtype, int norders)
{
	assert(rtype==rt_seed && r->land && r->land->trees[0] >= norders);
	r->land->trees[0] -= norders;
}

static int
limit_seeds(const region * r, const resource_type * rtype)
{
	assert(rtype==rt_seed);
	if(fval(r, RF_MALLORN)) return 0;
	return r->land?r->land->trees[0]:0;
}

void
init_seed(void)
{
  attrib * a;
  resource_limit * rdata;

  rt_seed = rt_find("seed");
  if (rt_seed!=NULL) {
    a = a_add(&rt_seed->attribs, a_new(&at_resourcelimit));
    rdata = (resource_limit*)a->data.v;
    rdata->limit = limit_seeds;
    rdata->produce = produce_seeds;
  }
}

/* mallorn */

static void
produce_mallornseeds(region * r, const resource_type * rtype, int norders)
{
  assert(rtype==rt_mallornseed && r->land && r->land->trees[0] >= norders);
  assert(fval(r, RF_MALLORN));
  r->land->trees[0] -= norders;
}

static int
limit_mallornseeds(const region * r, const resource_type * rtype)
{
  assert(rtype==rt_mallornseed);
  if (!fval(r, RF_MALLORN)) {
    return 0;
  }
  return r->land?r->land->trees[0]:0;
}

void
init_mallornseed(void)
{
  attrib * a;
  resource_limit * rdata;

  rt_mallornseed = rt_find("mallornseed");
  if (rt_mallornseed!=NULL) {
    rt_mallornseed->flags |= RTF_LIMITED;
    rt_mallornseed->flags |= RTF_POOLED;
	
    a = a_add(&rt_mallornseed->attribs, a_new(&at_resourcelimit));
    rdata = (resource_limit*)a->data.v;
    rdata->limit = limit_mallornseeds;
    rdata->produce = produce_mallornseeds;
  }
}
