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

#include <attributes/reduceproduction.h>
#include <spells/regioncurse.h>

/* kernel includes */
#include "curse.h"
#include "item.h"
#include "unit.h"
#include "faction.h"
#include "race.h"
#include "region.h"
#include "skill.h"
#include "terrain.h"
#include "lighthouse.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/gamedata.h>
#include <util/language.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/umlaut.h>

#include <storage.h>
#include <selist.h>
#include <critbit.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct building_typelist {
    struct building_typelist *next;
    building_type *type;
} building_typelist;

selist *buildingtypes = NULL;
static critbit_tree cb_bldgtypes;

/* Returns a building type for the (internal) name */
static building_type *bt_find_i(const char *name)
{
    const char *match;
    building_type *btype = NULL;

    match = cb_find_str(&cb_bldgtypes, name);
    if (match) {
        cb_get_kv(match, &btype, sizeof(btype));
    }
    return btype;
}

const building_type *bt_find(const char *name)
{
    building_type *btype = bt_find_i(name);
    if (!btype) {
        log_warning("bt_find: could not find building '%s'\n", name);
    }
    return btype;
}

static int bt_changes = 1;

bool bt_changed(int *cache)
{
    assert(cache);
    if (*cache != bt_changes) {
        *cache = bt_changes;
        return true;
    }
    return false;
}

static void bt_register(building_type * btype)
{
    size_t len;
    char data[64];

    selist_push(&buildingtypes, (void *)btype);
    len = cb_new_kv(btype->_name, strlen(btype->_name), &btype, sizeof(btype), data);
    assert(len <= sizeof(data));
    cb_insert(&cb_bldgtypes, data, len);
    ++bt_changes;
}

static void free_buildingtype(void *ptr) {
    building_type *btype = (building_type *)ptr;
    free_construction(btype->construction);
    free(btype->maintenance);
    free(btype->_name);
    free(btype);
}

void free_buildingtypes(void) {
    cb_clear(&cb_bldgtypes);
    selist_foreach(buildingtypes, free_buildingtype);
    selist_free(buildingtypes);
    buildingtypes = 0;
    ++bt_changes;
}

building_type *bt_get_or_create(const char *name)
{
    assert(name && name[0]);
    if (name != NULL) {
        building_type *btype = bt_find_i(name);
        if (btype == NULL) {
            btype = calloc(sizeof(building_type), 1);
            btype->_name = strdup(name);
            btype->auraregen = 1.0;
            btype->maxsize = -1;
            btype->capacity = 1;
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
            return MIN(b->type->maxcapacity, b->size * b->type->capacity);
        }
        return b->size * b->type->capacity;
    }
    if (building_finished(b)) {
        if (b->type->maxcapacity >= 0) {
            return b->type->maxcapacity;
        }
    }
    return 0;
}

attrib_type at_building_generic_type = {
    "building_generic_type", NULL, NULL, NULL, a_writestring, a_readstring, NULL,
    ATF_UNIQUE
};

/* TECH DEBT: simplest thing that works for E3 dwarf/halfling faction rules */
static int adjust_size(const building *b, int bsize) {
    assert(b);
    if (config_get_int("rules.dwarf_castles", 0)
        && strcmp(b->type->_name, "castle") == 0) {
        unit *u = building_owner(b);
        if (u && u->faction->race == get_race(RC_HALFLING)) {
            return bsize * 5 / 4;
        }
    }
    return bsize;
}

/* Returns the (internal) name for a building of given size and type. Especially, returns the correct
 * name if it depends on the size (as for Eressea castles).
 */
const char *buildingtype(const building_type * btype, const building * b, int bsize)
{
    const construction *con;

    assert(btype);

    if (b && b->attribs) {
        if (is_building_type(btype, "generic")) {
            const attrib *a = a_find(b->attribs, &at_building_generic_type);
            if (a) {
                return (const char *)a->data.v;
            }
        }
    }
    if (btype->construction && btype->construction->name) {
        if (b) {
            bsize = adjust_size(b, bsize);
        }
        for (con = btype->construction; con; con = con->improvement) {
            bsize -= con->maxsize;
            if (!con->improvement || bsize <0) {
                return con->name;
            }
        }
    }
    return btype->_name;
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
        selist *ql = buildingtypes;
        int qi;

        bn = (local_names *)calloc(sizeof(local_names), 1);
        bn->next = bnames;
        bn->lang = lang;

        for (qi = 0, ql = buildingtypes; ql; selist_advance(&ql, &qi, 1)) {
            building_type *btype = (building_type *)selist_get(ql, qi);

            const char *n = LOC(lang, btype->_name);
            if (!n) {
                log_error("building type %s has no translation in %s",
                          btype->_name, locale_name(lang));
            } else {
                type.v = (void *)btype;
                addtoken((struct tnode **)&bn->names, n, type);
            }
        }
        bnames = bn;
    }
    if (findtoken(bn->names, name, &type) == E_TOK_NOMATCH)
        return NULL;
    return (const building_type *)type.v;
}

int cmp_castle_size(const building * b, const building * a)
{
    if (!b || !(b->type->flags & BTF_FORTIFICATION) || !building_owner(b)) {
        return -1;
    }
    if (!a || !(a->type->flags & BTF_FORTIFICATION) || !building_owner(a)) {
        return 1;
    }
    return b->size - a->size;
}

static const int castle_bonus[6] = { 0, 1, 3, 5, 8, 12 };
static const int watch_bonus[3] = { 0, 1, 2 };

int building_protection(const building_type * btype, int stage)
{
    assert(btype->flags & BTF_FORTIFICATION);
    if (btype->maxsize < 0) {
        return castle_bonus[MIN(stage, 5)];
    }
    return watch_bonus[MIN(stage, 2)];
}

void write_building_reference(const struct building *b, struct storage *store)
{
    WRITE_INT(store, (b && b->region) ? b->no : 0);
}

void resolve_building(building *b)
{
    resolve(RESOLVE_BUILDING | b->no, b);
}

int read_building_reference(gamedata * data, building **bp, resolve_fun fun)
{
    int id;
    READ_INT(data->store, &id);
    if (id > 0) {
        *bp = findbuilding(id);
        if (*bp == NULL) {
            *bp = NULL;
            ur_add(RESOLVE_BUILDING | id, (void**)bp, fun);
        }
    }
    else {
        *bp = NULL;
    }
    return id;
}

building *new_building(const struct building_type * btype, region * r,
    const struct locale * lang)
{
    building **bptr = &r->buildings;
    building *b = (building *)calloc(1, sizeof(building));
    const char *bname = 0;
    char buffer[32];

    b->no = newcontainerid();
    bhash(b);

    b->type = btype;
    b->region = r;
    while (*bptr)
        bptr = &(*bptr)->next;
    *bptr = b;

    update_lighthouse(b);
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
    slprintf(buffer, sizeof(buffer), "%s %s", bname, itoa36(b->no));
    b->name = strdup(bname);
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

    /* Falls Karawanserei, Damm oder Tunnel einst�rzen, wird die schon
     * gebaute Strasse zur Haelfte vernichtet */
    /* TODO: caravan, tunnel, dam modularization ? is_building_type ? */
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
    int n = 0;
    const construction *cons = btype->construction;

    if (b) {
        bsize = adjust_size(b, bsize);
    }

    if (!cons) {
        return 0;
    }

    while (cons && cons->maxsize != -1 && bsize >= cons->maxsize) {
        bsize -= cons->maxsize;
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
    /* Eigent�mer tot oder kein Eigent�mer vorhanden. Erste lebende Einheit
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
    if (!heir && config_token("rules.region_owner_pay_building", bld->type->_name)) {
        if (rule_region_owners()) {
            u = building_owner(largestbuilding(bld->region, cmp_taxes, false));
        }
        else {
            u = building_owner(largestbuilding(bld->region, cmp_wage, false));
        }
        if (u) {
            heir = u;
        }
    }
    return heir;
}

unit *building_owner(const building * bld)
{
    unit *owner;
    if (!bld) {
        return NULL;
    }
    owner = bld->_owner;
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
        self->name = strdup(name);
    else
        self->name = NULL;
}

region *building_getregion(const building * b)
{
    return b->region;
}

bool
buildingtype_exists(const region * r, const building_type * bt, bool working)
{
    building *b;

    for (b = rbuildings(r); b; b = b->next) {
        if (b->type == bt && (!working || fval(b, BLD_MAINTAINED)) && building_finished(b)) {
            return true;
        }
    }

    return false;
}

bool building_finished(const struct building *b) {
    return b->size >= b->type->maxsize;
}

bool building_is_active(const struct building *b) {
    return b && fval(b, BLD_MAINTAINED) && building_finished(b);
}

building *active_building(const unit *u, const struct building_type *btype) {
    if (u->building && u->building->type == btype && building_is_active(u->building)) {
        return inside_building(u);
    }
    return 0;
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

bool in_safe_building(unit *u1, unit *u2) {
    if (u1->building) {
        building * b = inside_building(u1);
        if (b && b->type->flags & BTF_FORTIFICATION) {
            if (!u2->building) {
                return true;
            }
            if (u2->building != b || b != inside_building(u2)) {
                return true;
            }
        }
    }
    return false;
}

bool is_building_type(const struct building_type *btype, const char *name) {
    assert(btype);
    return name && strcmp(btype->_name, name)==0;
}

building *largestbuilding(const region * r, cmp_building_cb cmp_gt,
    bool imaginary)
{
    building *b, *best = NULL;

    for (b = r->buildings; b; b = b->next) {
        if (cmp_gt(b, best) <= 0)
            continue;
        if (!imaginary) {
            const attrib *a = a_find(b->attribs, &at_icastle);
            if (a)
                continue;
        }
        best = b;
    }
    return best;
}
/* Lohn bei den einzelnen Burgstufen f�r Normale Typen, Orks, Bauern,
 * Modifikation f�r St�dter. */

static const int wagetable[7][4] = {
    { 10, 10, 11, -7 },             /* Baustelle */
    { 10, 10, 11, -5 },             /* Handelsposten */
    { 11, 11, 12, -3 },             /* Befestigung */
    { 12, 11, 13, -1 },             /* Turm */
    { 13, 12, 14, 0 },              /* Burg */
    { 14, 12, 15, 1 },              /* Festung */
    { 15, 13, 16, 2 }               /* Zitadelle */
};

static int
default_wage(const region * r, const faction * f, const race * rc, int in_turn)
{
    building *b = largestbuilding(r, cmp_wage, false);
    int esize = 0;
    double wage;

    if (b != NULL) {
        /* TODO: this reveals imaginary castles */
        esize = buildingeffsize(b, false);
    }

    if (f != NULL) {
        int index = 0;
        if (rc == get_race(RC_ORC) || rc == get_race(RC_SNOTLING)) {
            index = 1;
        }
        wage = wagetable[esize][index];
    }
    else {
        if (is_mourning(r, in_turn)) {
            wage = 10;
        }
        else if (fval(r->terrain, SEA_REGION)) {
            wage = 11;
        }
        else {
            wage = wagetable[esize][2];
        }
        if (r->attribs && rule_blessed_harvest() == HARVEST_WORK) {
            /* E1 rules */
            wage += harvest_effect(r);
        }
    }

    if (r->attribs) {
        attrib *a;
        curse *c;

        /* Godcurse: Income -10 */
        c = get_curse(r->attribs, &ct_godcursezone);
        if (c && curse_active(c)) {
            wage = MAX(0, wage - 10);
        }

        /* Bei einer D�rre verdient man nur noch ein Viertel  */
        c = get_curse(r->attribs, &ct_drought);
        if (c && curse_active(c)) {
            wage /= curse_geteffect(c);
        }

        a = a_find(r->attribs, &at_reduceproduction);
        if (a) {
            wage = (wage * a->data.sa[0]) / 100;
        }
    }
    return (int)wage;
}

static int
minimum_wage(const region * r, const faction * f, const race * rc, int in_turn)
{
    if (f && rc) {
        return rc->maintenance;
    }
    return default_wage(r, f, rc, in_turn);
}

/* Gibt Arbeitslohn f�r entsprechende Rasse zur�ck, oder f�r
* die Bauern wenn f == NULL. */
int wage(const region * r, const faction * f, const race * rc, int in_turn)
{
    static int config;
    static int rule_wage;
    if (config_changed(&config)) {
        rule_wage = config_get_int("rules.wage.function", 1);
    }
    if (rule_wage==0) {
        return 0;
    }
    if (rule_wage==1) {
        return default_wage(r, f, rc, in_turn);
    }
    return minimum_wage(r, f, rc, in_turn);
}

int cmp_wage(const struct building *b, const building * a)
{
    if (is_building_type(b->type, "castle")) {
        if (!a)
            return 1;
        if (b->size > a->size)
            return 1;
        if (b->size == a->size)
            return 0;
    }
    return -1;
}

int building_taxes(const building *b) {
    assert(b);
    return b->type->taxes;
}

int cmp_taxes(const building * b, const building * a)
{
    faction *f = region_get_owner(b->region);
    if (b->type->taxes) {
        unit *u = building_owner(b);
        if (!u) {
            return -1;
        }
        else if (a) {
            int newtaxes = building_taxes(b);
            int oldtaxes = building_taxes(a);

            if (newtaxes > oldtaxes)
                return -1;
            else if (newtaxes < oldtaxes)
                return 1;
            else if (b->size < a->size)
                return -1;
            else if (b->size > a->size)
                return 1;
            else {
                if (u && u->faction == f) {
                    u = building_owner(a);
                    if (u && u->faction == f) {
                        return 0;
                    }
                    return 1;
                }
            }
        }
        else {
            return 1;
        }
    }
    return 0;
}

int cmp_current_owner(const building * b, const building * a)
{
    faction *f = region_get_owner(b->region);

    assert(rule_region_owners());
    if (f && b->type->taxes) {
        unit *u = building_owner(b);
        if (!u || u->faction != f)
            return -1;
        if (a) {
            int newtaxes = building_taxes(b);
            int oldtaxes = building_taxes(a);

            if (newtaxes > oldtaxes) {
                return 1;
            }
            if (newtaxes < oldtaxes) {
                return -1;
            }
            return (b->size - a->size);
        }
        else {
            return 1;
        }
    }
    return 0;
}
