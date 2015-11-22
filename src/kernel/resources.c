/*
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
#include "resources.h"

/* kernel includes */
#include "build.h"
#include "item.h"
#include "region.h"
#include "terrain.h"

#include <util/rand.h>
#include <util/rng.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

static double ResourceFactor(void)
{
    static double value = -1.0;
    if (value < 0) {
        const char *str = config_get("resource.factor");
        value = str ? atof(str) : 1.0;
    }
    return value;
}

void update_resources(region * r)
{
    struct rawmaterial *res = r->resources;
    while (res) {
        if (res->type->update)
            res->type->update(res, r);
        res = res->next;
    }
}

extern int dice_rand(const char *s);

static void update_resource(struct rawmaterial *res, double modifier)
{
    double amount = 1 + (res->level - res->startlevel) * res->divisor / 100.0;
    amount = ResourceFactor() * res->base * amount * modifier;
    if (amount < 1.0)
        res->amount = 1;
    else
        res->amount = (int)amount;
    assert(res->amount > 0);
}

void
add_resource(region * r, int level, int base, int divisor,
const resource_type * rtype)
{
    struct rawmaterial *rm = calloc(sizeof(struct rawmaterial), 1);

    rm->next = r->resources;
    r->resources = rm;
    rm->level = level;
    rm->startlevel = level;
    rm->base = base;
    rm->divisor = divisor;
    rm->flags = 0;
    rm->type = rmt_get(rtype);
    update_resource(rm, 1.0);
    rm->type->terraform(rm, r);
}

void terraform_resources(region * r)
{
    int i;
    const terrain_type *terrain = r->terrain;
    static int terraform_all = -1;
    if (terraform_all < 0) {
        terraform_all = config_get_int("rules.terraform.all", 0);
    }

    if (terrain->production == NULL)
        return;
    for (i = 0; terrain->production[i].type; ++i) {
        rawmaterial *rm;
        const terrain_production *production = terrain->production + i;
        const resource_type *rtype = production->type;

        for (rm = r->resources; rm; rm = rm->next) {
            if (rm->type->rtype == rtype)
                break;
        }
        if (rm) {
            continue;
        }

        if (terraform_all || chance(production->chance)) {
            add_resource(r, dice_rand(production->startlevel),
                dice_rand(production->base), dice_rand(production->divisor),
                production->type);
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
    unused_arg(r);
}

#ifdef RANDOM_CHANGE
static void resource_random_change(int *pvalue, bool used)
{
    int split = 5;
    int rnd = rng_int() % 100;

    if (pvalue == 0 || rnd % 10 >= 10)
        return;
    if (used)
        split = 4;
    /* if a resource was mined this round, there is a 6% probability
     * of a decline and a 4% probability of a raise. */
    /* if it wasn't mined this round, there is an equal probability
     * of 5% for a decline or a raise. */
    if (rnd < split) {
        (*pvalue)++;
    } else {
        (*pvalue)--;
    }
    if ((*pvalue) < 0)
        (*pvalue) = 0;
}
#endif

static int visible_default(const rawmaterial * res, int skilllevel)
/* resources are visible, if skill equals minimum skill to mine them
 * plus current level of difficulty */
{
    const struct item_type *itype = res->type->rtype->itype;
    if (res->level <= 1
        && res->level + itype->construction->minskill <= skilllevel + 1) {
        assert(res->amount > 0);
        return res->amount;
    }
    else if (res->level + itype->construction->minskill <= skilllevel + 2) {
        assert(res->amount > 0);
        return res->amount;
    }
    return -1;
}

static void use_default(rawmaterial * res, const region * r, int amount)
{
    assert(res->amount > 0 && amount >= 0 && amount <= res->amount);
    res->amount -= amount;
    while (res->amount == 0) {
        double modifier =
            1.0 + ((rng_int() % (SHIFT * 2 + 1)) - SHIFT) * ((rng_int() % (SHIFT * 2 +
            1)) - SHIFT) / 10000.0;
        int i;

        for (i = 0; r->terrain->production[i].type; ++i) {
            if (res->type->rtype == r->terrain->production[i].type)
                break;
        }

        ++res->level;
        update_resource(res, modifier);
    }
}

struct rawmaterial *rm_get(region * r, const struct resource_type *rtype)
{
    struct rawmaterial *rm = r->resources;
    while (rm && rm->type->rtype != rtype)
        rm = rm->next;
    return rm;
}

struct rawmaterial_type *rawmaterialtypes = 0;

struct rawmaterial_type *rmt_find(const char *str)
{
    rawmaterial_type *rmt = rawmaterialtypes;
    while (rmt && strcmp(rmt->name, str) != 0)
        rmt = rmt->next;
    return rmt;
}

struct rawmaterial_type *rmt_get(const struct resource_type *rtype)
{
    rawmaterial_type *rmt = rawmaterialtypes;
    while (rmt && rmt->rtype != rtype)
        rmt = rmt->next;
    return rmt;
}

struct rawmaterial_type *rmt_create(const struct resource_type *rtype,
    const char *name)
{
    rawmaterial_type *rmtype = malloc(sizeof(rawmaterial_type));
    rmtype->name = _strdup(name);
    rmtype->rtype = rtype;
    rmtype->terraform = terraform_default;
    rmtype->update = NULL;
    rmtype->use = use_default;
    rmtype->visible = visible_default;
    rmtype->next = rawmaterialtypes;
    rawmaterialtypes = rmtype;
    return rmtype;
}
