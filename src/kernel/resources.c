#include "resources.h"

/* kernel includes */
#include <kernel/config.h>
#include <kernel/callbacks.h>
#include "build.h"
#include "item.h"
#include "region.h"
#include "terrain.h"

#include <util/macros.h>
#include <util/rand.h>
#include <util/rng.h>

#include <stb_ds.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

static double ResourceFactor(void)
{
    return config_get_flt("resource.factor", 1.0);
}

void update_resources(region * r)
{
    ptrdiff_t len = arrlen(r->resources);
    for (ptrdiff_t i = 0; i != len; ++i) {
        rawmaterial* res = r->resources + i;
        struct rawmaterial_type *raw = rmt_get(res->rtype);
        if (raw && raw->update) {
            raw->update(res, r);
        }
    }
}

static void update_resource(struct rawmaterial *res, double modifier)
{
    double amount = (res->level - res->startlevel) / 100.0 * res->divisor + 1;
    amount = ResourceFactor() * res->base * amount * modifier;
    if (amount < 1.0)
        res->amount = 1;
    else
        res->amount = (int)amount;
    assert(res->amount > 0);
}

void set_resource(struct rawmaterial *rm, int level, int base, int divisor)
{
    rm->level = level;
    rm->startlevel = level;
    rm->base = base;
    rm->amount = base;
    rm->divisor = divisor;
}

struct rawmaterial *
add_resource(region * r, int level, int base, int divisor,
const resource_type * rtype)
{    
    struct rawmaterial *rm = arraddnptr(r->resources, 1);
    rm->flags = 0;
    rm->rtype = rtype;
    set_resource(rm, level, base, divisor);
    return rm;
}

void terraform_resources(region * r)
{
    int i;
    const terrain_type *terrain = r->terrain;

    if (terrain->production == NULL) {
        return;
    }
    if (r->resources != NULL) {
        arrfree(r->resources);
    }
    for (i = 0; terrain->production[i].type; ++i) {
        
        const terrain_production *production = terrain->production + i;

        if (chance(production->chance)) {
            rawmaterial_type *raw;
            rawmaterial* rm = add_resource(r, dice_rand(production->startlevel),
                dice_rand(production->base), dice_rand(production->divisor),
                production->type);
            update_resource(rm, 1.0);
            raw = rmt_get(rm->rtype);
            if (raw && raw->terraform) {
                raw->terraform(rm, r);
            }
        }
    }
}

static void terraform_default(struct rawmaterial *res, const region * r)
{
#define SHIFT 70
    double modifier =
        1.0 + ((rng_int() % (SHIFT * 2 + 1)) - SHIFT) * ((rng_int() % (SHIFT * 2 +
        1)) - SHIFT) / 10000.0;
    res->amount = (int)(res->amount * modifier);  /* random adjustment, +/- 91% */
    if (res->amount < 1)
        res->amount = 1;
    UNUSED_ARG(r);
}

static int visible_default(const rawmaterial * res, int skilllevel)
/* resources are visible if skill equals minimum skill to mine them
 * plus current level of difficulty */
{
    const struct item_type *itype = res->rtype->itype;
    int level = res->level + itype->construction->minskill - 1;
    if (res->level <= 1
        && level <= skilllevel) {
        assert(res->amount > 0);
        return res->amount;
    }
    else if (level < skilllevel) {
        assert(res->amount > 0);
        return res->amount;
    }
    return -1;
}

static int visible_half_skill(const rawmaterial * res, int skilllevel)
/* resources are visible if skill equals half as much as normal */
{
    const struct item_type *itype = res->rtype->itype;
    int level = res->level + itype->construction->minskill - 1;
    if (2 * skilllevel >= level) {
        return res->amount;
    }
    return -1;
}

static void use_default(rawmaterial * res, const region * r, int amount)
{
    assert(res->amount > 0 && amount >= 0 && amount <= res->amount);
    res->amount -= amount;
    while (res->amount == 0) {
        long rn = ((rng_int() % (SHIFT * 2 + 1)) - SHIFT) * ((rng_int() % (SHIFT * 2 + 1)) - SHIFT);
        double modifier = 1.0 + rn / 10000.0;
        int i;

        for (i = 0; r->terrain->production[i].type; ++i) {
            if (res->rtype == r->terrain->production[i].type)
                break;
        }

        ++res->level;
        update_resource(res, modifier);
    }
}

struct rawmaterial *rm_get(region * r, const struct resource_type *rtype)
{
    ptrdiff_t i, len = arrlen(r->resources);
    assert(rtype);
    for (i = 0; i != len; ++i) {
        rawmaterial *rm = r->resources + i;
        if (rm->rtype == rtype) {
            return rm;
        }
    }
    return NULL;
}

struct rawmaterial_type *rmt_get(const struct resource_type *rtype)
{
    return rtype->raw;
}

struct rawmaterial_type *rmt_create(struct resource_type *rtype)
{
    if (!rtype->raw) {
        int rule = config_get_int("resource.visibility.rule", 1);
        rawmaterial_type *rmtype = rtype->raw = malloc(sizeof(rawmaterial_type));
        if (!rmtype) abort();
        rmtype->rtype = rtype;
        rmtype->terraform = terraform_default;
        rmtype->update = NULL;
        rmtype->use = use_default;
        if (rule == 0) {
            rmtype->visible = visible_default;
        }
        else {
            rmtype->visible = visible_half_skill;
        }
    }
    return rtype->raw;
}

int limit_resource(const struct region *r, const resource_type *rtype)
{
    assert(!rtype->raw);
    if (callbacks.limit_resource) {
        return callbacks.limit_resource(r, rtype);
    }
    return -1;
}

void produce_resource(struct region *r, const struct resource_type *rtype, int amount)
{
    assert(!rtype->raw);
    if (callbacks.produce_resource) {
        callbacks.produce_resource(r, rtype, amount);
    }
}
