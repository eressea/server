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
#include <kernel/attrib.h>
#include <util/base36.h>
#include <kernel/event.h>
#include <util/functions.h>
#include <kernel/gamedata.h>
#include <util/language.h>
#include <util/log.h>
#include <util/param.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/strings.h>
#include <util/umlaut.h>

#include <storage.h>
#include <selist.h>
#include <critbit.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
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
    while (btype->stages) {
        building_stage *next = btype->stages->next;
        free(btype->stages->name);
        free(btype->stages);
        btype->stages = next;
    }
    free(btype->maintenance);
    free(btype->_name);
    free(btype);
}

building_type *bt_get_or_create(const char *name)
{
    assert(name && name[0]);
    if (name != NULL) {
        building_type *btype = bt_find_i(name);
        if (btype == NULL) {
            btype = (building_type *)calloc(1, sizeof(building_type));
            if (!btype) abort();
            btype->_name = str_strdup(name);
            btype->flags = BTF_DEFAULT;
            btype->auraregen = 1.0;
            btype->maxsize = -1;
            btype->capacity = 1;
            btype->maxcapacity = -1;
            btype->magres = frac_zero;
            bt_register(btype);
        }
        return btype;
    }
    return NULL;
}

int buildingcapacity(const building * b)
{
    if (b->type->capacity >= 0) {
        int cap = b->size * b->type->capacity;
        if (b->type->maxcapacity > 0 && b->type->maxcapacity < cap) {
            cap = b->type->maxcapacity;
        }
        return cap;
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
    assert(btype);

    if (b && b->attribs) {
        if (is_building_type(btype, "generic")) {
            const attrib *a = a_find(b->attribs, &at_building_generic_type);
            if (a) {
                return (const char *)a->data.v;
            }
        }
    }
    if (btype->stages && btype->stages->name) {
        const building_stage *stage;
        if (b) {
            bsize = adjust_size(b, bsize);
        }
        for (stage = btype->stages; stage; stage = stage->next) {
            bsize -= stage->construction.maxsize;
            if (!stage->next || bsize <0) {
                return stage->name;
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

static void free_bnames(void) {
    while (bnames) {
        local_names *bn = bnames;
        bnames = bnames->next;
        if (bn->names) {
            freetokens(bn->names);
        }
        free(bn);
    }
}

static local_names *get_bnames(const struct locale *lang)
{
    static int config;
    local_names *bn;

    if (bt_changed(&config)) {
        free_bnames();
    }
    bn = bnames;
    while (bn) {
        if (bn->lang == lang) {
            break;
        }
        bn = bn->next;
    }
    if (!bn) {
        selist *ql = buildingtypes;
        int qi;

        bn = (local_names *)calloc(1, sizeof(local_names));
        if (!bn) abort();
        bn->next = bnames;
        bn->lang = lang;

        for (qi = 0, ql = buildingtypes; ql; selist_advance(&ql, &qi, 1)) {
            building_type *btype = (building_type *)selist_get(ql, qi);

            const char *n = LOC(lang, btype->_name);
            if (!n) {
                log_error("building type %s has no translation in %s",
                    btype->_name, locale_name(lang));
            }
            else {
                variant type;
                type.v = (void *)btype;
                addtoken((struct tnode **)&bn->names, n, type);
            }
        }
        bnames = bn;
    }
    return bn;
}

/* Find the building type for a given localized name (as seen by the user). Useful for parsing
 * orders. The inverse of locale_string(lang, btype->_name), sort of. */
const building_type *findbuildingtype(const char *name,
    const struct locale *lang)
{
    variant type;
    local_names *bn = get_bnames(lang);
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

static const int castle_bonus[7] = { 0, 0, 1, 2, 3, 4, 5 };
static const int watch_bonus[3] = { 0, 1, 2 };

int bt_protection(const building_type * btype, int stage)
{
    if (btype->flags & BTF_FORTIFICATION) {
        if (btype->maxsize < 0) {
            if (stage >= 7) {
                stage = 6;
            }
            return castle_bonus[stage];
        }
        if (stage > 2) {
            stage = 2;
        }
        return watch_bonus[stage];
    }
    return 0;
}

int building_protection(const building* b) {
    return bt_protection(b->type, buildingeffsize(b, false));
}

void write_building_reference(const struct building *b, struct storage *store)
{
    WRITE_INT(store, (b && b->region) ? b->no : 0);
}

void resolve_building(building *b)
{
    resolve(RESOLVE_BUILDING | b->no, b);
}

int read_building_reference(gamedata * data, building **bp)
{
    int id;
    READ_INT(data->store, &id);
    if (id > 0) {
        *bp = findbuilding(id);
        if (*bp == NULL) {
            *bp = building_create(id);
        }
    }
    else {
        *bp = NULL;
    }
    return id;
}

building *building_create(int id)
{
    building *b = (building *)calloc(1, sizeof(building));
    if (!b) abort();
    b->no = id;
    bhash(b);
    return b;
}

static int newbuildingid(void) {
    int random_no;
    int start_random_no;

    random_no = 1 + (rng_int() % MAX_CONTAINER_NR);
    start_random_no = random_no;

    while (findbuilding(random_no)) {
        random_no++;
        if (random_no == MAX_CONTAINER_NR + 1) {
            random_no = 1;
        }
        if (random_no == start_random_no) {
            random_no = (int)MAX_CONTAINER_NR + 1;
        }
    }
    return random_no;
}

building *new_building(const struct building_type * btype, region * r,
    const struct locale * lang, int size)
{
    building **bptr = &r->buildings;
    int id = newbuildingid();
    building *b = building_create(id);
    const char *bname;
    char buffer[32];

    assert(size > 0);
    b->type = btype;
    b->region = r;
    while (*bptr)
        bptr = &(*bptr)->next;
    *bptr = b;

    bname = LOC(lang, btype->_name);
    if (!bname) {
        bname = param_name(P_GEBAEUDE, lang);
    }
    assert(bname);
    snprintf(buffer, sizeof(buffer), "%s %s", bname, itoa36(b->no));
    b->name = str_strdup(buffer);
    b->size = size;
    return b;
}

static building *deleted_buildings;

/** remove a building from the region.
 * remove_building lets units leave the building
 */
void remove_building(building ** blist, building * b)
{
    unit *u;
    region *r = b->region;
    static const struct building_type *bt_caravan, *bt_dam, *bt_tunnel;
    static int btypes;

    assert(bfindhash(b->no));

    if (bt_changed(&btypes)) {
        bt_caravan = bt_find("caravan");
        bt_dam = bt_find("dam");
        bt_tunnel = bt_find("tunnel");
    }
    handle_event(b->attribs, "destroy", b);
    for (u = r->units; u; u = u->next) {
        if (u->building == b) leave(u, true);
    }

    b->size = 0;
    bunhash(b);

    /* Falls Karawanserei, Damm oder Tunnel einstuerzen, wird die schon
     * gebaute Strasse zur Haelfte vernichtet */
    if (b->type == bt_caravan || b->type == bt_dam || b->type == bt_tunnel) {
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
int buildingeffsize(const building * b, bool imaginary)
{
    const struct building_type *btype = NULL;

    if (b == NULL)
        return 0;

    if (imaginary) {
        const attrib *a = a_find(b->attribs, &at_icastle);
        if (a) {
            btype = (const struct building_type *)a->data.v;
        }
    }
    else {
        btype = b->type;
    }
    return bt_effsize(btype, b, b->size);
}

const building_type *visible_building(const building *b) {
    if (b->attribs) {
        const attrib *a = a_find(b->attribs, &at_icastle);
        if (a != NULL) {
            return icastle_type(a);
        }
    }
    return b->type;
}

int bt_effsize(const building_type * btype, const building * b, int bsize)
{
    if (b) {
        bsize = adjust_size(b, bsize);
    }

    if (btype && btype->stages) {
        int n = 0;
        const building_stage *stage = btype->stages;
        do {
            const construction *con = &stage->construction;
            if (con->maxsize < 0) {
                break;
            }
            else {
                if (bsize >= con->maxsize) {
                    bsize -= con->maxsize;
                    ++n;
                }
                stage = stage->next;
            }
        } while (stage && bsize > 0);
        return n;
    }

    return 0;
}

const char *write_buildingname(const building * b, char *ibuf, size_t size)
{
    snprintf(ibuf, size, "%s (%s)", b->name, itoa36(b->no));
    return ibuf;
}

const char *buildingname(const building * b)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;
    char *ibuf = idbuf[(++nextbuf) % 8];
    return write_buildingname(b, ibuf, sizeof(idbuf[0]));
}

void building_set_owner(struct unit * owner)
{
    assert(owner && owner->building);
    owner->building->_owner = owner;
}

static unit *building_owner_ex(const building * bld, const struct faction * last_owner)
{
    unit *u, *heir = NULL;
    /* Eigentuemer tot oder kein Eigentuemer vorhanden.
     * Erste lebende Einheit nehmen. */
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
    bld->_owner = building_owner_ex(bld, owner ? owner->faction : NULL);
}

const char *building_getname(const building * self)
{
    return self->name;
}

void building_setname(building * self, const char *name)
{
    free(self->name);
    if (name)
        self->name = str_strdup(name);
    else
        self->name = NULL;
}

region *building_getregion(const building * b)
{
    return b->region;
}

building *
get_building_of_type(const region * r, const building_type * bt, bool working)
{
    building *b;

    for (b = rbuildings(r); b; b = b->next) {
        if (b->type == bt && !(working && fval(b, BLD_UNMAINTAINED)) && building_finished(b)) {
            return b;
        }
    }

    return NULL;
}

bool
buildingtype_exists(const region* r, const building_type* bt, bool working)
{
    building* b = get_building_of_type(r, bt, working);
    return b != NULL;
}

bool building_finished(const struct building *b) {
    return b->size >= b->type->maxsize;
}

bool building_is_active(const struct building *b) {
    return b && !fval(b, BLD_UNMAINTAINED) && building_finished(b);
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
        if (b) {
            const building_type *btype = visible_building(b);
            if (btype->flags & BTF_FORTIFICATION) {
                if (!u2->building) {
                    return true;
                }
                if (u2->building != b || b != inside_building(u2)) {
                    return true;
                }
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
/* Lohn bei den einzelnen Burgstufen fuer Normale Typen, Orks, Bauern */

static const int wagetable[7][3] = {
    { 10, 10, 11 },             /* Baustelle */
    { 10, 10, 11 },             /* Handelsposten */
    { 11, 11, 12 },             /* Befestigung */
    { 12, 11, 13 },             /* Turm */
    { 13, 12, 14 },             /* Burg */
    { 14, 12, 15 },             /* Festung */
    { 15, 13, 16 }              /* Zitadelle */
};

static int
default_wage(const region * r, const race * rc)
{
    building *b = largestbuilding(r, cmp_wage, false);
    int esize = 0;
    int wage;

    if (b != NULL) {
        /* TODO: this reveals imaginary castles */
        esize = buildingeffsize(b, false);
    }

    if (rc != NULL) {
        static const struct race *rc_orc, *rc_snotling;
        static int rc_cache;
        int index = 0;
        if (rc_changed(&rc_cache)) {
            rc_orc = get_race(RC_ORC);
            rc_snotling = get_race(RC_SNOTLING);
        }
        if (rc == rc_orc || rc == rc_snotling) {
            index = 1;
        }
        wage = wagetable[esize][index];
    }
    else {
        wage = wagetable[esize][2];
        if (rule_blessed_harvest() & HARVEST_WORK) {
            /* Geändert in E3 */
            wage += harvest_effect(r);
        }
    }

    if (r->attribs) {
        attrib *a;
        curse *c;
        variant vm;
     
        /* Godcurse: Income -10 */
        vm = frac_make(wage, 1);

        /* Bei einer Duerre verdient man nur noch ein Viertel  */
        c = get_curse(r->attribs, &ct_drought);
        if (c && curse_active(c)) {
            vm = frac_mul(vm, frac_make(1, curse_geteffect_int(c)));
        }

        a = a_find(r->attribs, &at_reduceproduction);
        if (a) {
            vm = frac_mul(vm, frac_make(a->data.sa[0], 100));
        }
        wage = vm.sa[0] / vm.sa[1];
    }
    return wage;
}

static int
minimum_wage(const region * r, const race * rc)
{
    if (rc) {
        return rc->maintenance;
    }
    return default_wage(r, rc);
}

/**
 * Gibt Arbeitslohn fuer entsprechende Rasse zurueck, oder fuer
 * die Bauern wenn rc == NULL. */
int wage(const region * r, const race * rc)
{
    static int config;
    static int rule_wage;
    if (config_changed(&config)) {
        rule_wage = config_get_int("rules.wage.function", 1);
    }
    if (rule_wage == 0) {
        return 0;
    }

    if (rule_wage == 1) {
        return default_wage(r, rc);
    }
    return minimum_wage(r, rc);
}

int peasant_wage(const struct region *r, bool mourn)
{
    return mourn ? 10 : wage(r, NULL);
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

void free_buildingtypes(void) {
    free_bnames();
    cb_clear(&cb_bldgtypes);
    selist_foreach(buildingtypes, free_buildingtype);
    selist_free(buildingtypes);
    buildingtypes = 0;
    ++bt_changes;
}
