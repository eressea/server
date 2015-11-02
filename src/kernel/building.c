/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>

#include <kernel/config.h>
#include "building.h"

/* kernel includes */
#include "item.h"
#include "curse.h"              /* für C_NOCOST */
#include "unit.h"
#include "faction.h"
#include "race.h"
#include "region.h"
#include "skill.h"
#include "save.h"
#include "lighthouse.h"
#include "version.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/log.h>
#include <quicklist.h>
#include <util/resolve.h>
#include <util/umlaut.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* attributes includes */
#include <attributes/matmod.h>

typedef struct building_typelist {
    struct building_typelist *next;
    building_type *type;
} building_typelist;

quicklist *buildingtypes = NULL;

/* Returns a building type for the (internal) name */
static building_type *bt_find_i(const char *name)
{
    quicklist *ql;
    int qi;

    assert(name);

    for (qi = 0, ql = buildingtypes; ql; ql_advance(&ql, &qi, 1)) {
        building_type *btype = (building_type *)ql_get(ql, qi);
        if (strcmp(btype->_name, name) == 0)
            return btype;
    }
    return NULL;
}

const building_type *bt_find(const char *name)
{
    return bt_find_i(name);
}

void bt_register(building_type * type)
{
    if (type->init) {
        type->init(type);
    }
    ql_push(&buildingtypes, (void *)type);
}

void free_buildingtype(void *ptr) {
    building_type *btype = (building_type *)ptr;
    free_construction(btype->construction);
    free(btype->maintenance);
    free(btype->_name);
    free(btype);
}

void free_buildingtypes(void) {
    ql_foreach(buildingtypes, free_buildingtype);
    ql_free(buildingtypes);
    buildingtypes = 0;
}

building_type *bt_get_or_create(const char *name)
{
    if (name != NULL) {
        building_type *btype = bt_find_i(name);
        if (btype == NULL) {
            btype = calloc(sizeof(building_type), 1);
            btype->_name = _strdup(name);
            btype->auraregen = 1.0;
            btype->maxsize = -1;
            btype->capacity = -1;
            btype->maxcapacity = -1;
            bt_register(btype);
        }
        return btype;
    }
    return NULL;
}

int buildingcapacity(const building * b)
{
    if (b->type->capacity >= 0) {
        if (b->type->maxcapacity >= 0) {
            return _min(b->type->maxcapacity, b->size * b->type->capacity);
        }
        return b->size * b->type->capacity;
    }
    if (b->size >= b->type->maxsize) {
        if (b->type->maxcapacity >= 0) {
            return b->type->maxcapacity;
        }
    }
    return 0;
}

attrib_type at_building_generic_type = {
    "building_generic_type", NULL, NULL, NULL, a_writestring, a_readstring,
    ATF_UNIQUE
};

/* Returns the (internal) name for a building of given size and type. Especially, returns the correct
 * name if it depends on the size (as for Eressea castles).
 */
const char *buildingtype(const building_type * btype, const building * b, int bsize)
{
    const char *s;
    assert(btype);

    s = btype->_name;
    if (btype->name) {
        s = btype->name(btype, b, bsize);
    }
    if (b && b->attribs) {
        const struct building_type *bt_generic = bt_find("generic");

        if (btype == bt_generic) {
            const attrib *a = a_find(b->attribs, &at_building_generic_type);
            if (a) {
                s = (const char *)a->data.v;
            }
        }
    }
    return s;
}

#define BMAXHASH 7919
static building *buildhash[BMAXHASH];
void bhash(building * b)
{
    building *old = buildhash[b->no % BMAXHASH];

    buildhash[b->no % BMAXHASH] = b;
    b->nexthash = old;
}

void bunhash(building * b)
{
    building **show;

    for (show = &buildhash[b->no % BMAXHASH]; *show; show = &(*show)->nexthash) {
        if ((*show)->no == b->no)
            break;
    }
    if (*show) {
        assert(*show == b);
        *show = (*show)->nexthash;
        b->nexthash = 0;
    }
}

static building *bfindhash(int i)
{
    building *old;

    for (old = buildhash[i % BMAXHASH]; old; old = old->nexthash)
        if (old->no == i)
            return old;
    return 0;
}

building *findbuilding(int i)
{
    return bfindhash(i);
}

/* ** old building types ** */

static int sm_smithy(const unit * u, const region * r, skill_t sk, int value)
{                               /* skillmod */
    if (sk == SK_WEAPONSMITH || sk == SK_ARMORER) {
        if (u->region == r)
            return value + 1;
    }
    return value;
}

static int mm_smithy(const unit * u, const resource_type * rtype, int value)
{                               /* material-mod */
    if (rtype == get_resourcetype(R_IRON))
        return value * 2;
    return value;
}

static void init_smithy(struct building_type *bt)
{
    a_add(&bt->attribs, make_skillmod(NOSKILL, SMF_PRODUCTION, sm_smithy, 1.0,
        0));
    a_add(&bt->attribs, make_matmod(mm_smithy));
}

static const char *castle_name_i(const struct building_type *btype,
    const struct building *b, int bsize, const char *fname[])
{
    int i = bt_effsize(btype, b, bsize);

    return fname[i];
}

static const char *castle_name_2(const struct building_type *btype,
    const struct building *b, int bsize)
{
    const char *fname[] = {
        "site",
        "fortification",
        "tower",
        "castle",
        "fortress",
        "citadel"
    };
    return castle_name_i(btype, b, bsize, fname);
}

static const char *castle_name(const struct building_type *btype,
    const struct building *b, int bsize)
{
    const char *fname[] = {
        "site",
        "tradepost",
        "fortification",
        "tower",
        "castle",
        "fortress",
        "citadel"
    };
    return castle_name_i(btype, b, bsize, fname);
}

static const char *fort_name(const struct building_type *btype,
    const struct building *b, int bsize)
{
    const char *fname[] = {
        "scaffolding",
        "guardhouse",
        "guardtower",
    };
    return castle_name_i(btype, b, bsize, fname);
}

/* for finding out what was meant by a particular building string */

static local_names *bnames;

/* Find the building type for a given localized name (as seen by the user). Useful for parsing
 * orders. The inverse of locale_string(lang, btype->_name), sort of. */
const building_type *findbuildingtype(const char *name,
    const struct locale *lang)
{
    variant type;
    local_names *bn = bnames;

    while (bn) {
        if (bn->lang == lang)
            break;
        bn = bn->next;
    }
    if (!bn) {
        quicklist *ql = buildingtypes;
        int qi;

        bn = (local_names *)calloc(sizeof(local_names), 1);
        bn->next = bnames;
        bn->lang = lang;

        for (qi = 0, ql = buildingtypes; ql; ql_advance(&ql, &qi, 1)) {
            building_type *btype = (building_type *)ql_get(ql, qi);

            const char *n = LOC(lang, btype->_name);
            type.v = (void *)btype;
            addtoken(&bn->names, n, type);
        }
        bnames = bn;
    }
    if (findtoken(bn->names, name, &type) == E_TOK_NOMATCH)
        return NULL;
    return (const building_type *)type.v;
}

static int building_protection(building * b, unit * u, building_bonus bonus)
{

    int i = 0;
    int bsize = buildingeffsize(b, false);
    const construction *cons = b->type->construction;
    if (!cons) {
        return 0;
    }

    for (i = 0; i < bsize; i++)
    {
        cons = cons->improvement;
    }

    switch (bonus)
    {
    case DEFENSE_BONUS:
        return cons->defense_bonus;
    case CLOSE_COMBAT_ATTACK_BONUS:
        return cons->close_combat_bonus;
    case RANGED_ATTACK_BONUS:
        return cons->ranged_bonus;
    default:
        return 0;
    }
}

static int meropis_building_protection(building * b, unit * u)
{
    return 2;
}

void register_buildings(void)
{
    register_function((pf_generic)& building_protection,
        "building_protection");
    register_function((pf_generic)& meropis_building_protection,
        "meropis_building_protection");
    register_function((pf_generic)& init_smithy, "init_smithy");
    register_function((pf_generic)& castle_name, "castle_name");
    register_function((pf_generic)& castle_name_2, "castle_name_2");
    register_function((pf_generic)& fort_name, "fort_name");
}

void write_building_reference(const struct building *b, struct storage *store)
{
    WRITE_INT(store, (b && b->region) ? b->no : 0);
}

int resolve_building(variant id, void *address)
{
    int result = 0;
    building *b = NULL;
    if (id.i != 0) {
        b = findbuilding(id.i);
        if (b == NULL) {
            result = -1;
        }
    }
    *(building **)address = b;
    return result;
}

variant read_building_reference(struct storage * store)
{
    variant result;
    READ_INT(store, &result.i);
    return result;
}

building *new_building(const struct building_type * btype, region * r,
    const struct locale * lang)
{
    building **bptr = &r->buildings;
    building *b = (building *)calloc(1, sizeof(building));
    static bool init_lighthouse = false;
    static const struct building_type *bt_lighthouse = 0;
    const char *bname = 0;
    char buffer[32];

    if (!init_lighthouse) {
        bt_lighthouse = bt_find("lighthouse");
        init_lighthouse = true;
    }

    b->no = newcontainerid();
    bhash(b);

    b->type = btype;
    b->region = r;
    while (*bptr)
        bptr = &(*bptr)->next;
    *bptr = b;

    if (b->type == bt_lighthouse) {
        r->flags |= RF_LIGHTHOUSE;
    }
    if (b->type->name) {
        bname = LOC(lang, buildingtype(btype, b, 0));
    }
    if (!bname) {
        bname = LOC(lang, btype->_name);
    }
    if (!bname) {
        bname = LOC(lang, parameters[P_GEBAEUDE]);
    }
    if (!bname) {
        bname = parameters[P_GEBAEUDE];
    }
    assert(bname);
    slprintf(buffer, sizeof(buffer), "%s %s", bname, buildingid(b));
    b->name = _strdup(bname);
    return b;
}

static building *deleted_buildings;

/** remove a building from the region.
 * remove_building lets units leave the building
 */
void remove_building(building ** blist, building * b)
{
    unit *u;
    const struct building_type *bt_caravan, *bt_dam, *bt_tunnel;

    assert(bfindhash(b->no));

    bt_caravan = bt_find("caravan");
    bt_dam = bt_find("dam");
    bt_tunnel = bt_find("tunnel");

    handle_event(b->attribs, "destroy", b);
    for (u = b->region->units; u; u = u->next) {
        if (u->building == b) leave(u, true);
    }

    b->size = 0;
    update_lighthouse(b);
    bunhash(b);

    /* Falls Karawanserei, Damm oder Tunnel einstürzen, wird die schon
     * gebaute Straße zur Hälfte vernichtet */
    if (b->type == bt_caravan || b->type == bt_dam || b->type == bt_tunnel) {
        region *r = b->region;
        int d;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            direction_t dir = (direction_t)d;
            if (rroad(r, dir) > 0) {
                rsetroad(r, dir, rroad(r, dir) / 2);
            }
        }
    }

    /* Stattdessen nur aus Liste entfernen, aber im Speicher halten. */
    while (*blist && *blist != b) {
        blist = &(*blist)->next;
    }
    *blist = b->next;
    b->region = NULL;
    b->next = deleted_buildings;
    deleted_buildings = b;
}

void free_building(building * b)
{
    while (b->attribs)
        a_remove(&b->attribs, b->attribs);
    free(b->name);
    free(b->display);
    free(b);
}

void free_buildings(void)
{
    while (deleted_buildings) {
        building *b = deleted_buildings;
        deleted_buildings = b->next;
    }
}

extern struct attrib_type at_icastle;

/** returns the building's build stage (NOT size in people).
 * only makes sense for castles or similar buildings with multiple
 * stages */
int buildingeffsize(const building * b, int img)
{
    const struct building_type *btype = NULL;

    if (b == NULL)
        return 0;

    btype = b->type;
    if (img) {
        const attrib *a = a_find(b->attribs, &at_icastle);
        if (a) {
            btype = (const struct building_type *)a->data.v;
        }
    }
    return bt_effsize(btype, b, b->size);
}

int bt_effsize(const building_type * btype, const building * b, int bsize)
{
    int i = bsize, n = 0;
    const construction *cons = btype->construction;

    /* TECH DEBT: simplest thing that works for E3 dwarf/halfling faction rules */
    if (b && get_param_int(global.parameters, "rules.dwarf_castles", 0)
        && strcmp(btype->_name, "castle") == 0) {
        unit *u = building_owner(b);
        if (u && u->faction->race == get_race(RC_HALFLING)) {
            i = bsize * 10 / 8;
        }
    }

    if (!cons || !cons->improvement) {
        return 0;
    }

    while (cons && cons->maxsize != -1 && i >= cons->maxsize) {
        i -= cons->maxsize;
        cons = cons->improvement;
        ++n;
    }

    return n;
}

const char *write_buildingname(const building * b, char *ibuf, size_t size)
{
    slprintf(ibuf, size, "%s (%s)", b->name, itoa36(b->no));
    return ibuf;
}

const char *buildingname(const building * b)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;
    char *ibuf = idbuf[(++nextbuf) % 8];
    return write_buildingname(b, ibuf, sizeof(name));
}

void building_set_owner(struct unit * owner)
{
    assert(owner && owner->building);
    owner->building->_owner = owner;
}

static unit *building_owner_ex(const building * bld, const struct faction * last_owner)
{
    unit *u, *heir = 0;
    /* Eigentümer tot oder kein Eigentümer vorhanden. Erste lebende Einheit
      * nehmen. */
    for (u = bld->region->units; u; u = u->next) {
        if (u->building == bld) {
            if (u->number > 0) {
                if (heir && last_owner && heir->faction != last_owner && u->faction == last_owner) {
                    heir = u;
                    break; /* we found someone from the same faction who is not dead. let's take this guy */
                }
                else if (!heir) {
                    heir = u; /* you'll do in an emergency */
                }
            }
        }
    }
    if (!heir && check_param(global.parameters, "rules.region_owner_pay_building", bld->type->_name)) {
        if (rule_region_owners()) {
            u = building_owner(largestbuilding(bld->region, &cmp_taxes, false));
        }
        else {
            u = building_owner(largestbuilding(bld->region, &cmp_wage, false));
        }
        if (u) {
            heir = u;
        }
    }
    return heir;
}

unit *building_owner(const building * bld)
{
    if (!bld) {
        return NULL;
    }
    unit *owner = bld->_owner;
    if (!owner || (owner->building != bld || owner->number <= 0)) {
        unit * heir = building_owner_ex(bld, owner ? owner->faction : 0);
        return (heir && heir->number > 0) ? heir : 0;
    }
    return owner;
}

void building_update_owner(building * bld) {
    unit * owner = bld->_owner;
    bld->_owner = building_owner_ex(bld, owner ? owner->faction : 0);
}

const char *building_getname(const building * self)
{
    return self->name;
}

void building_setname(building * self, const char *name)
{
    free(self->name);
    if (name)
        self->name = _strdup(name);
    else
        self->name = NULL;
}

region *building_getregion(const building * b)
{
    return b->region;
}

bool building_is_active(const struct building *b) {
    return b && fval(b, BLD_WORKING);
}

void building_setregion(building * b, region * r)
{
    building **blist = &b->region->buildings;
    while (*blist && *blist != b) {
        blist = &(*blist)->next;
    }
    *blist = b->next;
    b->next = NULL;

    blist = &r->buildings;
    while (*blist && *blist != b)
        blist = &(*blist)->next;
    *blist = b;

    b->region = r;
}
