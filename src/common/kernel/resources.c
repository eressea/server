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
#include "eressea.h"
#include "resources.h"

#if NEW_RESOURCEGROWTH
/* kernel includes */
#include "build.h"
#include "item.h"
#include "region.h"
#include "terrain.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

static double
ResourceFactor(void)
{
  static double value = -1.0;
  if (value<0) {
    const char * str = get_param(global.parameters, "resource.factor");
    value = str?atof(str):1.0;
  }
  return value;
}

void
update_resources(region * r)
{
  struct rawmaterial * res = r->resources;
  while (res) {
    if (res->type->update) res->type->update(res, r);
    res = res->next;
  }
}

extern int dice_rand(const char *s);

static void
update_resource(struct rawmaterial * res, double modifier)
{
  double amount = 1 + (res->level-res->startlevel) * res->divisor/100.0;
  amount = ResourceFactor() * res->base * amount * modifier;
  if (amount<1.0) res->amount = 1;
  else res->amount = (int)amount;
  assert(res->amount>0);
}

void
terraform_resources(region * r)
{
	int i;
	const terraindata_t * tdata = &terrain[rterrain(r)];

	for (i=0;i!=3 && tdata->rawmaterials[i].type; ++i) {
		rawmaterial *rm;

		for(rm=r->resources; rm; rm=rm->next) {
			if(rm->type == tdata->rawmaterials[i].type) break;
		}
		if(rm) continue;

		if (rand()%100 < tdata->rawmaterials[i].chance) {
			struct rawmaterial * res = calloc(sizeof(struct rawmaterial), 1);

			res->next = r->resources;
			r->resources = res;
			res->level      = dice_rand(tdata->rawmaterials[i].startlevel);
			res->startlevel = res->level;
			res->base       = dice_rand(tdata->rawmaterials[i].base);
			res->divisor    = dice_rand(tdata->rawmaterials[i].divisor);
			res->flags      = 0;
			res->type       = tdata->rawmaterials[i].type;
                        update_resource(res, 1.0);
			res->type->terraform(res, r);
		}
	}
}

static void
terraform_default(struct rawmaterial * res, const region * r)
{
#define SHIFT 70
	double modifier = 1.0 + ((rand() % (SHIFT*2+1)) - SHIFT) * ((rand() % (SHIFT*2+1)) - SHIFT) / 10000.0;
	res->amount = (int)(res->amount*modifier); /* random adjustment, +/- 91% */
	if (res->amount<1) res->amount=1;
	unused(r);
}

#ifdef RANDOM_CHANGE
static void
resource_random_change(int *pvalue, boolean used)
{
	int split = 5;
	int rnd = rand()%100;

	if (pvalue==0 || rnd %10 >= 10) return;
	if (used) split = 4;
	/* if a resource was mined this round, there is a 6% probability
	 * of a decline and a 4% probability of a raise. */
	/* if it wasn't mined this round, there is an equal probability
	 * of 5% for a decline or a raise. */
	if(rnd < split) {
		(*pvalue)++;
	} else {
		(*pvalue)--;
	}
	if ((*pvalue) < 0) (*pvalue) = 0;
}
#endif

static int
visible_default(const rawmaterial *res, int skilllevel)
/* resources are visible, if skill equals minimum skill to mine them
 * plus current level of difficulty */
{
	const struct item_type * itype = olditemtype[res->type->_itype];
	if (res->level<=1 && res->level + itype->construction->minskill <= skilllevel+1) {
		assert (res->amount>0);
		return res->amount;
	} else if (res->level + itype->construction->minskill <= skilllevel+2) {
		assert (res->amount>0);
		return res->amount;
	}
	return -1;
}

static void 
use_default(rawmaterial *res, const region * r, int amount)
{
  const terraindata_t * tdata = &terrain[rterrain(r)];
  assert(res->amount>0 && amount>=0 && amount <= res->amount);
  res->amount-=amount;
  while (res->amount==0) {
    double modifier = 1.0 + ((rand() % (SHIFT*2+1)) - SHIFT) * ((rand() % (SHIFT*2+1)) - SHIFT) / 10000.0;
    int i;

    for (i=0; i!=3; ++i) {
      const rawmaterial_type * rmtype = tdata->rawmaterials[i].type;
      assert(rmtype);
      if (rmtype==res->type) break;
    }

    ++res->level;
    update_resource(res, modifier);
  }
}

struct rawmaterial *
rm_get(region * r, const struct resource_type * rtype)
{
	struct rawmaterial * rm = r->resources;
	while (rm && rm->type->rtype!=rtype) rm = rm->next;
	return rm;
}

struct rawmaterial_type rm_stones = {
	"rm_stone",
	I_STONE, NULL,
	terraform_default,
	NULL,
	use_default,
	visible_default
};

struct rawmaterial_type rm_iron = {
	"rm_iron",
	I_IRON, NULL,
	terraform_default,
	NULL,
	use_default,
	visible_default
};

struct rawmaterial_type rm_laen = {
	"rm_laen",
	I_LAEN, NULL,
	terraform_default,
	NULL,
	use_default,
	visible_default
};

#if 0
static struct rawmaterial_type rm_mallorn = {
	"rm_mallorn",
	I_MALLORN, NULL,
	terraform_trees,
	update_trees,
	use_trees,
	NULL
};

static struct rawmaterial_type rm_trees = {
	"rm_trees",
	I_WOOD, NULL,
	terraform_trees,
	update_trees,
	use_trees,
	NULL
};
#endif


struct rawmaterial_type * rawmaterialtypes = 0;

struct rawmaterial_type * 
rmt_find(const char * str)
{
	rawmaterial_type * rmt = rawmaterialtypes;
	while (rmt && strcmp(rmt->name, str)!=0) rmt = rmt->next;
	return rmt;
}

static void
add_rawmaterial(struct rawmaterial_type * rmtype)
{
	rmtype->rtype = item2resource(olditemtype[rmtype->_itype]);
	rmtype->next = rawmaterialtypes;
	rawmaterialtypes = rmtype;
}

void
init_rawmaterials(void)
{
	add_rawmaterial(&rm_stones);
	add_rawmaterial(&rm_iron);
	add_rawmaterial(&rm_laen);
#if 0
	add_rawmaterial(&rm_wood);
	add_rawmaterial(&rm_mallorn);
#endif
}

#endif
