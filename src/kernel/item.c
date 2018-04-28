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
#include "item.h"

#include <attributes/key.h>

#include "alchemy.h"
#include "build.h"
#include "curse.h"
#include "faction.h"
#include "messages.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "skill.h"
#include "terrain.h"
#include "unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/functions.h>
#include <util/goodies.h>
#include <util/log.h>
#include <util/language.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/rng.h>
#include <util/strings.h>
#include <util/umlaut.h>

#include <critbit.h>
#include <storage.h>

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static critbit_tree inames[MAXLOCALES];
static critbit_tree rnames[MAXLOCALES];
static critbit_tree cb_resources;
luxury_type *luxurytypes;

#define RTYPENAMELEN 24
typedef struct rt_entry {
    char key[RTYPENAMELEN];
    struct resource_type *value;
} rt_entry;

static int res_changeaura(unit * u, const resource_type * rtype, int delta)
{
    assert(rtype != NULL);
    return change_spellpoints(u, delta);
}

static int res_changeperson(unit * u, const resource_type * rtype, int delta)
{
    assert(rtype != NULL || !"not implemented");
    if (u->number + delta >= 0) {
        scale_number(u, u->number + delta);
    }
    else {
        scale_number(u, 0);
    }
    return u->number;
}

static int res_changepermaura(unit * u, const resource_type * rtype, int delta)
{
    assert(rtype != NULL);
    return change_maxspellpoints(u, delta);
}

static int res_changehp(unit * u, const resource_type * rtype, int delta)
{
    assert(rtype != NULL);
    u->hp += delta;
    return u->hp;
}

static int res_changepeasants(unit * u, const resource_type * rtype, int delta)
{
    assert(rtype != NULL && u->region->land);
    rsetpeasants(u->region, rpeasants(u->region) + delta);
    return rpeasants(u->region);
}

static int golem_factor(const unit *u, const resource_type *rtype) {
    if (rtype == get_resourcetype(R_STONE) && (u_race(u)->ec_flags & ECF_STONEGOLEM)) {
        return GOLEM_STONE;
    }
    if (rtype == get_resourcetype(R_IRON) && (u_race(u)->ec_flags & ECF_IRONGOLEM)) {
        return GOLEM_IRON;
    }
    return 0;
}

static int res_changeitem(unit * u, const resource_type * rtype, int delta)
{
    int num;
    int gf = (delta > 0) ? 0 : golem_factor(u, rtype);
    if (gf > 0) {
        if (delta != 0) {
            int reduce = delta / gf;
            if (delta % gf != 0) {
                --reduce;
            }
            if (reduce) scale_number(u, u->number + reduce);
        }
        num = u->number * gf;
    }
    else {
        const item_type *itype = resource2item(rtype);
        item *i;
        assert(itype != NULL);
        i = i_change(&u->items, itype, delta);
        if (i == NULL)
            return 0;
        num = i->number;
    }
    return num;
}

const char *resourcename(const resource_type * rtype, int flags)
{
    if (!rtype)
        assert(rtype);
    if (rtype) {
        if (rtype->name)
            return rtype->name(rtype, flags);

        if (flags & NMF_APPEARANCE && rtype->itype) {
            int i = (flags & NMF_PLURAL) ? 1 : 0;
            const char * result = rtype->itype->_appearance[i];
            if (result) {
                return result;
            }
        }
        if (flags & NMF_PLURAL) {
            static char name[64]; /* FIXME: static return value */
            size_t len = strlen(rtype->_name);
            assert(len <= sizeof(name) - 3);
            memcpy(name, rtype->_name, len);
            strcpy(name + len, "_p");
            return name;
        }
        return rtype->_name;
    }
    return "none";
}

static int num_resources;

static void rt_register(resource_type * rtype)
{
    const char * name = rtype->_name;
    size_t len = strlen(name);
    rt_entry ent;

    if (len >= RTYPENAMELEN) {
        log_error("resource name is longer than %d bytes: %s",
                RTYPENAMELEN-1, name);
        len = RTYPENAMELEN-1;
    }
    ent.value = rtype;
    memset(ent.key, 0, RTYPENAMELEN);
    memcpy(ent.key, name, len); 
    cb_insert(&cb_resources, &ent, sizeof(ent));
    ++num_resources;
}

resource_type *rt_get_or_create(const char *name) {
    resource_type *rtype = rt_find(name);
    if (!rtype) {
        rtype = calloc(1, sizeof(resource_type));
        if (!rtype) {
            perror("resource_type allocation failed");
        }
        else {
            rtype->_name = str_strdup(name);
            rt_register(rtype);
        }
    }
    return rtype;
}

static const char *it_aliases[][2] = {
    { "Runenschwert", "runesword" },
    { "p1", "goliathwater" },
    { "p2", "lifepotion" },
    { "p4", "ointment" },
    { "p5", "peasantblood" },
    { "p8", "nestwarmth" },
    { "p14", "healing" },
    { "p12", "truthpotion" },
    { "diamond", "adamantium" },
    { "diamondaxe", "adamantiumaxe" },
    { "diamondplate", "adamantiumplate" },
    { "aoh", "ao_healing" },
    { NULL, NULL },
};

static const char *it_alias(const char *zname)
{
    int i;
    for (i = 0; it_aliases[i][0]; ++i) {
        if (strcmp(it_aliases[i][0], zname) == 0)
            return it_aliases[i][1];
    }
    return NULL;
}

item_type *it_find(const char *zname)
{
    resource_type *result = rt_find(zname);
    if (!result) {
        const char *name = it_alias(zname);
        if (name) {
            result = rt_find(name);
        }
    }
    return result ? result->itype : 0;
}

item_type *it_get_or_create(resource_type *rtype) {
    item_type * itype;
    assert(rtype);
    itype = it_find(rtype->_name);
    if (!itype) {
        itype = (item_type *)calloc(sizeof(item_type), 1);
    }
    itype->rtype = rtype;
    rtype->uchange = res_changeitem;
    rtype->itype = itype;
    rtype->flags |= RTF_ITEM;
    return itype;
}

static void lt_register(luxury_type * ltype)
{
    ltype->itype->rtype->ltype = ltype;
    ltype->next = luxurytypes;
    luxurytypes = ltype;
}

luxury_type *new_luxurytype(item_type * itype, int price)
{
    luxury_type *ltype;

    assert(resource2luxury(itype->rtype) == NULL);

    ltype = calloc(sizeof(luxury_type), 1);
    ltype->itype = itype;
    ltype->price = price;
    lt_register(ltype);

    return ltype;
}

weapon_type *new_weapontype(item_type * itype,
    int wflags, variant magres, const char *damage[], int offmod, int defmod,
    int reload, skill_t sk)
{
    weapon_type *wtype;

    assert(itype && (!itype->rtype || !resource2weapon(itype->rtype)));

    wtype = calloc(sizeof(weapon_type), 1);
    if (damage) {
        wtype->damage[0] = str_strdup(damage[0]);
        wtype->damage[1] = str_strdup(damage[1]);
    }
    wtype->defmod = defmod;
    wtype->flags |= wflags;
    wtype->itype = itype;
    wtype->magres = magres;
    wtype->offmod = offmod;
    wtype->reload = reload;
    wtype->skill = sk;
    itype->rtype->wtype = wtype;

    return wtype;
}

armor_type *new_armortype(item_type * itype, double penalty, variant magres,
    int prot, unsigned int flags)
{
    armor_type *atype;

    assert(itype->rtype->atype == NULL);

    atype = calloc(sizeof(armor_type), 1);

    atype->itype = itype;
    atype->penalty = penalty;
    atype->magres = magres;
    atype->prot = prot;
    atype->flags = flags;
    itype->rtype->atype = atype;

    return atype;
}

void it_set_appearance(item_type *itype, const char *appearance)
{
    assert(itype);
    assert(itype->rtype);
    if (appearance) {
        char plural[32];
        itype->_appearance[0] = str_strdup(appearance);
        snprintf(plural, sizeof(plural), "%s_p", appearance);
        itype->_appearance[1] = str_strdup(plural);
    } else {
        itype->_appearance[0] = 0;
        itype->_appearance[1] = 0;
    }
}

const resource_type *item2resource(const item_type * itype)
{
    return itype ? itype->rtype : NULL;
}

const item_type *resource2item(const resource_type * rtype)
{
    return rtype ? rtype->itype : NULL;
}

const weapon_type *resource2weapon(const resource_type * rtype)
{
    return rtype->wtype;
}

const luxury_type *resource2luxury(const resource_type * rtype)
{
    return rtype->ltype;
}

resource_type *rt_find(const char *name)
{
    const void *match;
    size_t len = strlen(name);

    if (len >= RTYPENAMELEN) {
        log_warning("resource name is longer than %d bytes: %s",
                RTYPENAMELEN-1, name);
        return NULL;
    }
    match = cb_find_str(&cb_resources, name);
    if (match) {
        const rt_entry *ent = (const rt_entry *)match;
        return ent->value;
    }
    return NULL;
}

item **i_find(item ** i, const item_type * it)
{
    while (*i && (*i)->type != it)
        i = &(*i)->next;
    return i;
}

item *const* i_findc(item *const* iter, const item_type * it)
{
    while (*iter && (*iter)->type != it) {
        iter = &(*iter)->next;
    }
    return iter;
}

int i_get(const item * i, const item_type * it)
{
    i = *i_find((item **)& i, it);
    if (i)
        return i->number;
    return 0;
}

item *i_add(item ** pi, item * i)
{
    assert(i && i->type && !i->next);
    while (*pi) {
        int d = strcmp((*pi)->type->rtype->_name, i->type->rtype->_name);
        if (d >= 0)
            break;
        pi = &(*pi)->next;
    }
    if (*pi && (*pi)->type == i->type) {
        (*pi)->number += i->number;
        assert((*pi)->number >= 0);
        i_free(i);
    }
    else {
        i->next = *pi;
        *pi = i;
    }
    return *pi;
}

void i_merge(item ** pi, item ** si)
{
    item *i = *si;
    while (i) {
        item *itmp;
        while (*pi) {
            int d = strcmp((*pi)->type->rtype->_name, i->type->rtype->_name);
            if (d >= 0)
                break;
            pi = &(*pi)->next;
        }
        if (*pi && (*pi)->type == i->type) {
            (*pi)->number += i->number;
            assert((*pi)->number >= 0);
            i_free(i_remove(&i, i));
        }
        else {
            itmp = i->next;
            i->next = *pi;
            *pi = i;
            i = itmp;
        }
    }
    *si = NULL;
}

item *i_change(item ** pi, const item_type * itype, int delta)
{
    assert(itype);
    while (*pi) {
        int d = strcmp((*pi)->type->rtype->_name, itype->rtype->_name);
        if (d >= 0)
            break;
        pi = &(*pi)->next;
    }
    if (!*pi || (*pi)->type != itype) {
        item *i;
        if (delta == 0)
            return NULL;
        i = i_new(itype, delta);
        i->next = *pi;
        *pi = i;
    }
    else {
        item *i = *pi;
        i->number += delta;
        if (i->number < 0) {
            log_error("serious accounting error. number of items is %d.", i->number);
            i->number = 0;
        }
        if (i->number == 0) {
            *pi = i->next;
            i_free(i);
            return NULL;
        }
    }
    return *pi;
}

item *i_remove(item ** pi, item * i)
{
    assert(i);
    while ((*pi)->type != i->type)
        pi = &(*pi)->next;
    assert(*pi);
    *pi = i->next;
    i->next = NULL;
    return i;
}

static item *icache;
static int icache_size;
#define ICACHE_MAX 100

void item_done(void) {
    icache_size = ICACHE_MAX;
    i_freeall(&icache);
    icache_size = 0;
}

void i_free(item * i)
{
    if (icache_size >= ICACHE_MAX) {
        free(i);
    }
    else {
        i->next = icache;
        icache = i;
        ++icache_size;
    }
}

void i_freeall(item ** i)
{
    item *in;

    while (*i) {
        in = (*i)->next;
        i_free(*i);
        *i = in;
    }
}

item *i_new(const item_type * itype, int size)
{
    item *i;
    if (icache_size > 0) {
        i = icache;
        icache = i->next;
        --icache_size;
    }
    else {
        i = malloc(sizeof(item));
    }
    assert(itype);
    i->next = NULL;
    i->type = itype;
    i->number = size;
    assert(i->number >= 0);
    return i;
}

const item_type *oldpotiontype[MAX_POTIONS + 1];

/*** alte items ***/

static const char *resourcenames[MAX_RESOURCES] = {
    "money", "aura", "permaura",
    "hp", "peasant",
    "sapling", "mallornsapling", 
    "tree", "mallorntree",
    "seed", "mallornseed",
    "iron", "stone", "horse", "ao_healing",
    "aots", "roi", "rop", "ao_chastity",
    "laen", "fairyboot", "aoc", "pegasus",
    "elvenhorse", "charger", "dolphin", "roqf", "trollbelt",
    "aurafocus", "sphereofinv", "magicbag",
    "magicherbbag", "dreameye", "lifepotion"
};

const resource_type *get_resourcetype(resource_t type) {
    static int update;
    static struct resource_type * rtypes[MAX_RESOURCES];
    const resource_type *rtype = NULL;
    if (update != num_resources) {
        memset(rtypes, 0, sizeof(resource_type *) * MAX_RESOURCES);
        update = num_resources;
    }
    else {
        rtype = rtypes[type];
    }
    if (!rtype) {
        rtype = rtypes[type] = rt_find(resourcenames[type]);
    }
    return rtype;
}

int get_item(const unit * u, const item_type *itype)
{
    const item *i = *i_findc(&u->items, itype);
    assert(!i || i->number >= 0);
    return i ? i->number : 0;
}

int set_item(unit * u, const item_type *itype, int value)
{
    item *i;

    assert(itype);
    i = *i_find(&u->items, itype);
    if (!i) {
        i = i_add(&u->items, i_new(itype, value));
    }
    else {
        i->number = value;
        assert(i->number >= 0);
    }
    return value;
}

#include "move.h"

static int
mod_elves_only(const unit * u, const region * r, skill_t sk, int value)
{
    if (u_race(u) == get_race(RC_ELF))
        return value;
    UNUSED_ARG(r);
    return -118;
}

static int
mod_dwarves_only(const unit * u, const region * r, skill_t sk, int value)
{
    UNUSED_ARG(r);
    if (u_race(u) == get_race(RC_DWARF) || (u_race(u)->ec_flags & ECF_IRONGOLEM)) {
        return value;
    }
    return -118;
}

void
register_item_give(int(*foo) (struct unit *, struct unit *,
const struct item_type *, int, struct order *), const char *name)
{
    register_function((pf_generic)foo, name);
}

void
register_item_use(int(*foo) (struct unit *, const struct item_type *, int,
struct order *), const char *name)
{
    register_function((pf_generic)foo, name);
}

static void init_oldpotions(void)
{
    const char *potionnames[MAX_POTIONS] = {
        "p0", "goliathwater", "lifepotion", "p3", "ointment", "peasantblood", "p6",
        "p7", "nestwarmth", "p9", "p10", "p11", "truthpotion", "healing"
    };
    int p;

    for (p = 0; p != MAX_POTIONS; ++p) {
        item_type *itype = it_find(potionnames[p]);
        if (itype != NULL) {
            oldpotiontype[p] = itype;
        }
    }
}

void init_resources(void)
{
    resource_type *rtype;

    /* there are resources that are special and must be hard-coded.
     * these are not items, but things like trees or hitpoints
     * which can be used in a construction recipe or as a spell ingredient.
     */

    /* special resources needed in report_region */
    rtype = rt_get_or_create(resourcenames[R_SILVER]);
    rtype->flags |= RTF_ITEM | RTF_POOLED;
    rtype->uchange = res_changeitem;
    rtype->itype = it_get_or_create(rtype);
    rtype->itype->weight = 1;

    rtype = rt_get_or_create(resourcenames[R_HORSE]);
    rtype->flags |= RTF_ITEM | RTF_LIMITED;
    rtype->itype = it_get_or_create(rtype);
    rtype->itype->flags |= ITF_ANIMAL | ITF_BIG;

    /* "special" spell components */
    rtype = rt_get_or_create(resourcenames[R_AURA]);
    rtype->uchange = res_changeaura;
    rtype = rt_get_or_create(resourcenames[R_PERMAURA]);
    rtype->uchange = res_changepermaura;
    rtype = rt_get_or_create(resourcenames[R_LIFE]);
    rtype->uchange = res_changehp;
    rtype = rt_get_or_create(resourcenames[R_PEASANT]);
    rtype->uchange = res_changepeasants;

    /* trees are important, too: */
    rt_get_or_create(resourcenames[R_SAPLING]);
    rt_get_or_create(resourcenames[R_TREE]);
    rt_get_or_create(resourcenames[R_MALLORN_SAPLING]);
    rt_get_or_create(resourcenames[R_MALLORN_TREE]);

    /* alte typen registrieren: */
    init_oldpotions();
}

int get_money(const unit * u)
{
    const struct resource_type *rtype = get_resourcetype(R_SILVER);
    const item *i;
    for (i = u->items; i; i = i->next) {
        if (i->type->rtype == rtype) {
            return i->number;
        }
    }
    return 0;
}

int set_money(unit * u, int v)
{
    const struct resource_type *rtype = get_resourcetype(R_SILVER);
    item **ip = &u->items;
    while (*ip && (*ip)->type->rtype != rtype) {
        ip = &(*ip)->next;
    }
    if ((*ip) == NULL && v) {
        i_add(&u->items, i_new(rtype->itype, v));
        return v;
    }
    if ((*ip) != NULL) {
        if (v) {
            (*ip)->number = v;
            assert((*ip)->number >= 0);
        }
        else {
            i_remove(ip, *ip);
        }
    }
    return v;
}

int change_money(unit * u, int v)
{
    const struct resource_type *rtype = get_resourcetype(R_SILVER);
    item **ip = &u->items;
    while (*ip && (*ip)->type->rtype != rtype) {
        ip = &(*ip)->next;
    }
    if ((*ip) == NULL && v) {
        i_add(&u->items, i_new(rtype->itype, v));
        return v;
    }
    if ((*ip) != NULL) {
        item *i = *ip;
        if (i->number + v != 0) {
            i->number += v;
            assert(i->number >= 0);
            return i->number;
        }
        else {
            i_free(i_remove(ip, *ip));
        }
    }
    return 0;
}

static int add_resourcename_cb(const void * match, const void * key,
    size_t keylen, void *data)
{
    struct locale * lang = (struct locale *)data;
    int i = locale_index(lang);
    critbit_tree * cb = rnames + i;
    resource_type *rtype = ((rt_entry *)match)->value;

    for (i = 0; i != 2; ++i) {
        char buffer[128];
        const char * name = LOC(lang, resourcename(rtype, (i == 0) ? 0 : NMF_PLURAL));

        if (name && transliterate(buffer, sizeof(buffer), name)) {
            size_t len = strlen(buffer);
            assert(len + sizeof(rtype) < sizeof(buffer));
            len = cb_new_kv(buffer, len, &rtype, sizeof(rtype), buffer);
            cb_insert(cb, buffer, len);
        }
    }
    return 0;
}

const resource_type *findresourcetype(const char *name, const struct locale *lang)
{
    int i = locale_index(lang);
    critbit_tree * cb = rnames + i;
    char buffer[128];

    if (transliterate(buffer, sizeof(buffer), name)) {
        void * match;
        if (!cb->root) {
            /* first-time initialization  of resource names for this locale */
            cb_foreach(&cb_resources, "", 0, add_resourcename_cb, (void *)lang);
        }
        if (cb_find_prefix(cb, buffer, strlen(buffer), &match, 1, 0)) {
            const resource_type * rtype = 0;
            cb_get_kv(match, (void*)&rtype, sizeof(rtype));
            return rtype;
        }
    }
    else {
        log_debug("findresourcetype: transliterate failed for '%s'\n", name);
    }
    return NULL;
}

attrib_type at_showitem = {
    "showitem"
};

static int add_itemname_cb(const void * match, const void * key,
    size_t keylen, void *data)
{
    struct locale * lang = (struct locale *)data;
    critbit_tree * cb = inames + locale_index(lang);
    resource_type *rtype = ((rt_entry *)match)->value;

    if (rtype->itype) {
        int i;
        for (i = 0; i != 2; ++i) {
            char buffer[128];
            const char * name = LOC(lang, resourcename(rtype, (i == 0) ? 0 : NMF_PLURAL));

            if (name && transliterate(buffer, sizeof(buffer), name)) {
                size_t len = strlen(buffer);
                assert(len + sizeof(rtype->itype) < sizeof(buffer));
                len = cb_new_kv(buffer, len, &rtype->itype, sizeof(rtype->itype), buffer);
                cb_insert(cb, buffer, len);
            }
        }
    }
    return 0;
}

const item_type *finditemtype(const char *name, const struct locale *lang)
{
    int i = locale_index(lang);
    critbit_tree * cb = inames + i;
    char buffer[128];

    assert(name);
    if (transliterate(buffer, sizeof(buffer), name)) {
        void * match;
        if (!cb->root) {
            /* first-time initialization  of item names for this locale */
            cb_foreach(&cb_resources, "", 0, add_itemname_cb, (void *)lang);
        }
        if (cb_find_prefix(cb, buffer, strlen(buffer), &match, 1, 0)) {
            const item_type * itype = 0;
            cb_get_kv(match, (void*)&itype, sizeof(itype));
            return itype;
        }
    }
    else {
        log_debug("finditemtype: transliterate failed for '%s'\n", name);
    }
    return 0;
}

item *item_spoil(const struct race *rc, int size)
{
    item *itm = NULL;

    if (rng_int() % 100 < RACESPOILCHANCE) {
        char spoilname[32];
        const item_type *itype;

        sprintf(spoilname, "%sspoil", rc->_name);
        itype = it_find(spoilname);
        if (itype != NULL) {
            i_add(&itm, i_new(itype, size));
        }
    }
    return itm;
}

void read_items(struct storage *store, item ** ilist)
{
    for (;;) {
        char ibuf[32];
        int i;
        READ_STR(store, ibuf, sizeof(ibuf));
        if (!strcmp("end", ibuf)) {
            break;
        }
        READ_INT(store, &i);
        if (ilist) {
            const item_type *itype;
            itype = it_find(ibuf);
            if (i <= 0) {
                log_error("data contains an entry with %d %s", i, ibuf);
            }
            else {
                if (itype && itype->rtype) {
                    i_change(ilist, itype, i);
                }
                else {
                    log_error("data contains unknown item type %s.", ibuf);
                }
                assert(itype && itype->rtype);
            }
        }
    }
}

void write_items(struct storage *store, item * ilist)
{
    item *itm;
    for (itm = ilist; itm; itm = itm->next) {
        assert(itm->number >= 0);
        if (itm->number) {
            WRITE_TOK(store, resourcename(itm->type->rtype, 0));
            WRITE_INT(store, itm->number);
        }
    }
    WRITE_TOK(store, "end");
}

static void free_itype(item_type *itype) {
    assert(itype);
    free_construction(itype->construction);
    free(itype->_appearance[0]);
    free(itype->_appearance[1]);
    free(itype);
}

void free_atype(armor_type *atype) {
    free(atype);
}

void free_wtype(weapon_type *wtype) {
    assert(wtype);
    free(wtype->damage[0]);
    free(wtype->damage[1]);
    free(wtype);
}

void free_rtype(resource_type *rtype) {
    assert(rtype);
    if (rtype->wtype) {
        free_wtype(rtype->wtype);
    }
    if (rtype->itype) {
        free_itype(rtype->itype);
    }
    if (rtype->atype) {
        free_atype(rtype->atype);
    }
    free(rtype->modifiers);
    free(rtype->raw);
    free(rtype->_name);
    free(rtype);
}

static int free_rtype_cb(const void * match, const void * key,
    size_t keylen, void *cbdata)
{
    resource_type *rtype = ((rt_entry *)match)->value;;
    free_rtype(rtype);
    return 0;
}

void free_resources(void)
{
    int i;

    memset((void *)oldpotiontype, 0, sizeof(oldpotiontype));
    while (luxurytypes) {
        luxury_type * next = luxurytypes->next;
        free(luxurytypes);
        luxurytypes = next;
    }
    cb_foreach(&cb_resources, "", 0, free_rtype_cb, 0);
    cb_clear(&cb_resources);
    ++num_resources;

    for (i = 0; i != MAXLOCALES; ++i) {
        cb_clear(inames + i);
        cb_clear(rnames + i);
    }
}

void register_resources(void)
{
    static bool registered = false;
    if (registered) return;
    registered = true;

    register_function((pf_generic)mod_elves_only, "mod_elves_only");
    register_function((pf_generic)mod_dwarves_only, "mod_dwarves_only");
    register_function((pf_generic)res_changeitem, "changeitem");
    register_function((pf_generic)res_changeperson, "changeperson");
    register_function((pf_generic)res_changepeasants, "changepeasants");
    register_function((pf_generic)res_changepermaura, "changepermaura");
    register_function((pf_generic)res_changehp, "changehp");
    register_function((pf_generic)res_changeaura, "changeaura");
}

void show_item(unit * u, const item_type * itype)
{
    faction * f = u->faction;
    attrib *a;

    a = a_find(f->attribs, &at_showitem);
    while (a && a->data.v != itype)
        a = a->next;
    if (!a) {
        a = a_add(&f->attribs, a_new(&at_showitem));
        a->data.v = (void *)itype;
    }
}

