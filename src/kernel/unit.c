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
#include "unit.h"

#include "ally.h"
#include "building.h"
#include "faction.h"
#include "group.h"
#include "connection.h"
#include "curse.h"
#include "item.h"
#include "move.h"
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "spell.h"
#include "spellbook.h"
#include "save.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"

#include <attributes/moved.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/stealth.h>

#include "guard.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/strings.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/variant.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FIND_FOREIGN_TEMP

int weight(const unit * u)
{
    int w = 0, n = 0, in_bag = 0;
    const resource_type *rtype = get_resourcetype(R_BAG_OF_HOLDING);
    item *itm;

    for (itm = u->items; itm; itm = itm->next) {
        w = itm->type->weight * itm->number;
        n += w;
        if (rtype && !fval(itm->type, ITF_BIG)) {
            in_bag += w;
        }
    }

    n += u->number * u_race(u)->weight;

    if (rtype) {
        w = i_get(u->items, rtype->itype) * BAGCAPACITY;
        if (w > in_bag) w = in_bag;
        n -= w;
    }

    return n;
}

attrib_type at_creator = {
    "creator"
    /* Rest ist NULL; temporaeres, nicht alterndes Attribut */
};

unit *findunit(int n)
{
    if (n <= 0) {
        return NULL;
    }
    return ufindhash(n);
}

unit *findunitr(const region * r, int n)
{
    unit *u;
    /* findunit regional! */
    assert(n > 0);
    u = ufindhash(n);
    return (u && u->region == r) ? u : 0;
}

// TODO: deprecated, replace with findunit(n)
unit *findunitg(int n, const region * hint)
{
    unused_arg(hint);
    /* Abfangen von Syntaxfehlern. */
    if (n <= 0)
        return NULL;

    /* findunit global! */
    hint = 0;
    return ufindhash(n);
}

#define UMAXHASH MAXUNITS
static unit *unithash[UMAXHASH];
static unit *delmarker = (unit *)unithash;     /* a funny hack */

#define HASH_STATISTICS 1
#if HASH_STATISTICS
static int hash_requests;
static int hash_misses;
#endif

void uhash(unit * u)
{
    int key = HASH1(u->no, UMAXHASH), gk = HASH2(u->no, UMAXHASH);
    while (unithash[key] != NULL && unithash[key] != delmarker
        && unithash[key] != u) {
        key = (key + gk) % UMAXHASH;
    }
    assert(unithash[key] != u || !"trying to add the same unit twice");
    unithash[key] = u;
}

void uunhash(unit * u)
{
    int key = HASH1(u->no, UMAXHASH), gk = HASH2(u->no, UMAXHASH);
    while (unithash[key] != NULL && unithash[key] != u) {
        key = (key + gk) % UMAXHASH;
    }
    assert(unithash[key] == u || !"trying to remove a unit that is not hashed");
    unithash[key] = delmarker;
}

unit *ufindhash(int uid)
{
    assert(uid >= 0);
#if HASH_STATISTICS
    ++hash_requests;
#endif
    if (uid >= 0) {
        int key = HASH1(uid, UMAXHASH), gk = HASH2(uid, UMAXHASH);
        while (unithash[key] != NULL && (unithash[key] == delmarker
            || unithash[key]->no != uid)) {
            key = (key + gk) % UMAXHASH;
#if HASH_STATISTICS
            ++hash_misses;
#endif
        }
        return unithash[key];
    }
    return NULL;
}

typedef struct buddy {
    struct buddy *next;
    int number;
    faction *faction;
    unit *unit;
} buddy;

static buddy *get_friends(const unit * u, int *numfriends)
{
    buddy *friends = 0;
    faction *f = u->faction;
    region *r = u->region;
    int number = 0;
    unit *u2;

    for (u2 = r->units; u2; u2 = u2->next) {
        if (u2->faction != f && u2->number > 0) {
            int allied = 0;
            if (config_get_int("rules.alliances", 0) != 0) {
                allied = (f->alliance && f->alliance == u2->faction->alliance);
            }
            else if (alliedunit(u, u2->faction, HELP_MONEY)
                && alliedunit(u2, f, HELP_GIVE)) {
                allied = 1;
            }
            if (allied) {
                buddy *nf, **fr = &friends;

                /* some units won't take stuff: */
                if (u_race(u2)->ec_flags & GETITEM) {
                    while (*fr && (*fr)->faction->no < u2->faction->no)
                        fr = &(*fr)->next;
                    nf = *fr;
                    if (nf == NULL || nf->faction != u2->faction) {
                        nf = malloc(sizeof(buddy));
                        nf->next = *fr;
                        nf->faction = u2->faction;
                        nf->unit = u2;
                        nf->number = 0;
                        *fr = nf;
                    }
                    else if (nf->faction == u2->faction
                        && !(u_race(u2)->ec_flags & ECF_KEEP_ITEM)) {
                        /* we don't like to gift it to units that won't give it back */
                        if ((u_race(nf->unit)->ec_flags & ECF_KEEP_ITEM)) {
                            nf->unit = u2;
                        }
                    }
                    nf->number += u2->number;
                    number += u2->number;
                }
            }
        }
    }
    if (numfriends)
        *numfriends = number;
    return friends;
}

/** give all items to friends or peasants.
 * this function returns 0 on success, or 1 if there are items that
 * could not be destroyed.
 */
int gift_items(unit * u, int flags)
{
    const struct resource_type *rsilver = get_resourcetype(R_SILVER);
    const struct resource_type *rhorse = get_resourcetype(R_HORSE);
    region *r = u->region;
    item **itm_p = &u->items;
    int retval = 0;
    int rule = rule_give();

    assert(u->region);

    if ((rule & GIVE_ONDEATH) == 0 || !u->faction || (u->faction->flags & FFL_QUIT) == 0) {
        if ((rule & GIVE_ALLITEMS) == 0 && (flags & GIFT_FRIENDS))
            flags -= GIFT_FRIENDS;
        if ((rule & GIVE_PEASANTS) == 0 && (flags & GIFT_PEASANTS))
            flags -= GIFT_PEASANTS;
        if ((rule & GIVE_SELF) == 0 && (flags & GIFT_SELF))
            flags -= GIFT_SELF;
    }

    if (u->items == NULL || fval(u_race(u), RCF_ILLUSIONARY))
        return 0;
    if ((u_race(u)->ec_flags & ECF_KEEP_ITEM))
        return 0;

    /* at first, I should try giving my crap to my own units in this region */
    if (u->faction && (u->faction->flags & FFL_QUIT) == 0 && (flags & GIFT_SELF)) {
        unit *u2, *u3 = NULL;
        for (u2 = r->units; u2; u2 = u2->next) {
            if (u2 != u && u2->faction == u->faction && u2->number > 0) {
                /* some units won't take stuff: */
                if (u_race(u2)->ec_flags & GETITEM) {
                    /* we don't like to gift it to units that won't give it back */
                    if (!(u_race(u2)->ec_flags & ECF_KEEP_ITEM)) {
                        i_merge(&u2->items, &u->items);
                        u->items = NULL;
                        break;
                    }
                    else {
                        u3 = u2;
                    }
                }
            }
        }
        if (u->items && u3) {
            /* if nobody else takes it, we give it to a unit that has issues */
            i_merge(&u3->items, &u->items);
            u->items = NULL;
        }
        if (u->items == NULL)
            return 0;
    }

    /* if I have friends, I'll try to give my stuff to them */
    if (u->faction && (flags & GIFT_FRIENDS)) {
        int number = 0;
        buddy *friends = get_friends(u, &number);

        while (friends) {
            struct buddy *nf = friends;
            unit *u2 = nf->unit;
            item *itm = u->items;
            while (itm != NULL) {
                const item_type *itype = itm->type;
                item *itn = itm->next;
                int n = itm->number;
                n = n * nf->number / number;
                if (n > 0) {
                    i_change(&u->items, itype, -n);
                    i_change(&u2->items, itype, n);
                }
                itm = itn;
            }
            number -= nf->number;
            friends = nf->next;
            free(nf);
        }
        if (u->items == NULL)
            return 0;
    }

    /* last, but not least, give money and horses to peasants */
    while (*itm_p) {
        item *itm = *itm_p;

        if (flags & GIFT_PEASANTS) {
            if (u->region->land) {
                if (itm->type->rtype == rsilver) {
                    rsetmoney(r, rmoney(r) + itm->number);
                    itm->number = 0;
                }
                else if (itm->type->rtype == rhorse) {
                    rsethorses(r, rhorses(r) + itm->number);
                    itm->number = 0;
                }
            }
        }
        if (itm->number > 0 && (itm->type->flags & ITF_NOTLOST)) {
            itm_p = &itm->next;
            retval = -1;
        }
        else {
            i_remove(itm_p, itm);
            i_free(itm);
        }
    }
    return retval;
}

/** remove the unit from the list of active units.
 * the unit is not actually freed, because there may still be references
 * dangling to it (from messages, for example). To free all removed units,
 * call free_units().
 * returns 0 on success, or -1 if unit could not be removed.
 */

static unit *deleted_units = NULL;

#define DMAXHASH 7919
#undef DMAXHASH // TODO: makes dfindhash slow!
#ifdef DMAXHASH
typedef struct dead {
    struct dead *nexthash;
    faction *f;
    int no;
} dead;

static dead *deadhash[DMAXHASH];

static void dhash(int no, faction * f)
{
    dead *hash = (dead *)calloc(1, sizeof(dead));
    dead *old = deadhash[no % DMAXHASH];
    hash->no = no;
    hash->f = f;
    deadhash[no % DMAXHASH] = hash;
    hash->nexthash = old;
}

faction *dfindhash(int no)
{
    dead *old;

    if (no < 0)
        return 0;

    for (old = deadhash[no % DMAXHASH]; old; old = old->nexthash) {
        if (old->no == no) {
            return old->f;
        }
    }
    return 0;
}
#else
struct faction *dfindhash(int no) {
    unit *u = deleted_units;
    while (u && u->no != no) {
        u = u->next;
    }
    return u ? u->faction : NULL;
}
#endif

int remove_unit(unit ** ulist, unit * u)
{
    int result;

    assert(ufindhash(u->no));
    handle_event(u->attribs, "destroy", u);

    result = gift_items(u, GIFT_SELF | GIFT_FRIENDS | GIFT_PEASANTS);
    if (result != 0) {
        make_zombie(u);
        return -1;
    }

    if (u->number) {
        set_number(u, 0);
    }
    leave(u, true);
    u->region = NULL;

    uunhash(u);
    if (ulist) {
        while (*ulist != u) {
            ulist = &(*ulist)->next;
        }
        assert(*ulist == u);
        *ulist = u->next;
    }

    if (u->faction && u->faction->units == u) {
        u->faction->units = u->nextF;
    }
    if (u->prevF) {
        u->prevF->nextF = u->nextF;
    }
    if (u->nextF) {
        u->nextF->prevF = u->prevF;
    }
    u->nextF = 0;
    u->prevF = 0;

    u->next = deleted_units;
    deleted_units = u;
#ifdef DMAXHASH
    dhash(u->no, u->faction);
#endif
    // u_setfaction(u, NULL);

    u->region = NULL;

    return 0;
}

unit *findnewunit(const region * r, const faction * f, int n)
{
    unit *u2;

    if (n == 0)
        return 0;

    for (u2 = r->units; u2; u2 = u2->next)
        if (u2->faction == f && ualias(u2) == n)
            return u2;
#ifdef FIND_FOREIGN_TEMP
    for (u2 = r->units; u2; u2 = u2->next)
        if (ualias(u2) == n)
            return u2;
#endif
    return 0;
}

/* ------------------------------------------------------------- */

/*********************/
/*   at_alias   */
/*********************/
attrib_type at_alias = {
    "alias",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

int ualias(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_alias);
    if (!a)
        return 0;
    return a->data.i;
}

int a_readprivate(attrib * a, void *owner, gamedata *data)
{
    struct storage *store = data->store;
    char lbuf[DISPLAYSIZE];
    READ_STR(store, lbuf, sizeof(lbuf));
    a->data.v = _strdup(lbuf);
    return (a->data.v) ? AT_READ_OK : AT_READ_FAIL;
}

/*********************/
/*   at_private   */
/*********************/
attrib_type at_private = {
    "private",
    DEFAULT_INIT,
    a_finalizestring,
    DEFAULT_AGE,
    a_writestring,
    a_readprivate
};

const char *u_description(const unit * u, const struct locale *lang)
{
    if (u->display && u->display[0]) {
        return u->display;
    }
    else if (u_race(u)->describe) {
        return u_race(u)->describe(u->_race, lang);
    }
    return NULL;
}

const char *uprivate(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_private);
    if (!a)
        return NULL;
    return a->data.v;
}

void usetprivate(unit * u, const char *str)
{
    attrib *a = a_find(u->attribs, &at_private);

    if (str == NULL || *str == 0) {
        if (a) {
            a_remove(&u->attribs, a);
        }
        return;
    }
    if (!a) {
        a = a_add(&u->attribs, a_new(&at_private));
    }
    if (a->data.v) {
        free(a->data.v);
    }
    a->data.v = _strdup(str);
}

/*********************/
/*   at_potionuser   */
/*********************/
/* Einheit BENUTZT einen Trank */
attrib_type at_potionuser = {
    "potionuser",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

void usetpotionuse(unit * u, const potion_type * ptype)
{
    attrib *a = a_find(u->attribs, &at_potionuser);
    if (!a)
        a = a_add(&u->attribs, a_new(&at_potionuser));
    a->data.v = (void *)ptype;
}

const potion_type *ugetpotionuse(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_potionuser);
    if (!a)
        return NULL;
    return (const potion_type *)a->data.v;
}

/*********************/
/*   at_target   */
/*********************/
attrib_type at_target = {
    "target",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

unit *utarget(const unit * u)
{
    attrib *a;
    if (!fval(u, UFL_TARGET))
        return NULL;
    a = a_find(u->attribs, &at_target);
    assert(a || !"flag set, but no target found");
    return (unit *)a->data.v;
}

void usettarget(unit * u, const unit * t)
{
    attrib *a = a_find(u->attribs, &at_target);
    if (!a && t)
        a = a_add(&u->attribs, a_new(&at_target));
    if (a) {
        if (!t) {
            a_remove(&u->attribs, a);
            freset(u, UFL_TARGET);
        }
        else {
            a->data.v = (void *)t;
            fset(u, UFL_TARGET);
        }
    }
}

/*********************/
/*   at_siege   */
/*********************/

void a_writesiege(const attrib * a, const void *owner, struct storage *store)
{
    struct building *b = (struct building *)a->data.v;
    write_building_reference(b, store);
}

int a_readsiege(attrib * a, void *owner, gamedata *data)
{
    int result = read_reference(&a->data.v, data, read_building_reference,
        resolve_building);
    if (result == 0 && !a->data.v) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

attrib_type at_siege = {
    "siege",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writesiege,
    a_readsiege
};

struct building *usiege(const unit * u)
{
    attrib *a;
    if (!fval(u, UFL_SIEGE))
        return NULL;
    a = a_find(u->attribs, &at_siege);
    assert(a || !"flag set, but no siege found");
    return (struct building *)a->data.v;
}

void usetsiege(unit * u, const struct building *t)
{
    attrib *a = a_find(u->attribs, &at_siege);
    if (!a && t)
        a = a_add(&u->attribs, a_new(&at_siege));
    if (a) {
        if (!t) {
            a_remove(&u->attribs, a);
            freset(u, UFL_SIEGE);
        }
        else {
            a->data.v = (void *)t;
            fset(u, UFL_SIEGE);
        }
    }
}

/*********************/
/*   at_contact   */
/*********************/
attrib_type at_contact = {
    "contact",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

void usetcontact(unit * u, const unit * u2)
{
    attrib *a = a_find(u->attribs, &at_contact);
    while (a && a->type == &at_contact && a->data.v != u2)
        a = a->next;
    if (a && a->type == &at_contact)
        return;
    a_add(&u->attribs, a_new(&at_contact))->data.v = (void *)u2;
}

bool ucontact(const unit * u, const unit * u2)
/* Prueft, ob u den Kontaktiere-Befehl zu u2 gesetzt hat. */
{
    attrib *ru;
    if (u->faction == u2->faction)
        return true;

    /* Explizites KONTAKTIERE */
    for (ru = a_find(u->attribs, &at_contact); ru && ru->type == &at_contact;
    ru = ru->next) {
        if (((unit *)ru->data.v) == u2) {
            return true;
        }
    }

    return false;
}

/***
** init & cleanup module
**/

void free_units(void)
{
    while (deleted_units) {
        unit *u = deleted_units;
        deleted_units = deleted_units->next;
        free_unit(u);
        free(u);
    }
}

void write_unit_reference(const unit * u, struct storage *store)
{
    WRITE_INT(store, (u && u->region) ? u->no : 0);
}

int resolve_unit(variant id, void *address)
{
    unit *u = NULL;
    if (id.i != 0) {
        u = findunit(id.i);
        if (u == NULL) {
            *(unit **)address = NULL;
            return -1;
        }
    }
    *(unit **)address = u;
    return 0;
}

variant read_unit_reference(gamedata *data)
{
    variant var;
    READ_INT(data->store, &var.i);
    return var;
}

int get_level(const unit * u, skill_t id)
{
    assert(id != NOSKILL);
    if (skill_enabled(id)) {
        skill *sv = u->skills;
        while (sv != u->skills + u->skill_size) {
            if (sv->id == id) {
                return sv->level;
            }
            ++sv;
        }
    }
    return 0;
}

void set_level(unit * u, skill_t sk, int value)
{
    skill *sv = u->skills;

    assert(sk != SK_MAGIC || !u->faction || u->number == 1 || fval(u->faction, FFL_NPC));
    if (!skill_enabled(sk))
        return;

    if (value == 0) {
        remove_skill(u, sk);
        return;
    }
    while (sv != u->skills + u->skill_size) {
        if (sv->id == sk) {
            sk_set(sv, value);
            return;
        }
        ++sv;
    }
    sk_set(add_skill(u, sk), value);
}

static int leftship_age(struct attrib *a, void *owner)
{
    /* must be aged, so it doesn't affect report generation (cansee) */
    unused_arg(a);
    unused_arg(owner);
    return AT_AGE_REMOVE;         /* remove me */
}

static attrib_type at_leftship = {
    "leftship", NULL, NULL, leftship_age
};

static attrib *make_leftship(struct ship *leftship)
{
    attrib *a = a_new(&at_leftship);
    a->data.v = leftship;
    return a;
}

void set_leftship(unit * u, ship * sh)
{
    a_add(&u->attribs, make_leftship(sh));
}

ship *leftship(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_leftship);

    /* Achtung: Es ist nicht garantiert, dass der Rueckgabewert zu jedem
     * Zeitpunkt noch auf ein existierendes Schiff zeigt! */

    if (a)
        return (ship *)(a->data.v);

    return NULL;
}

void u_set_building(unit * u, building * b)
{
    assert(!b || !u->building); /* you must leave first */
    u->building = b;
    if (b && (!b->_owner || b->_owner->number <= 0)) {
        building_set_owner(u);
    }
}

void u_set_ship(unit * u, ship * sh)
{
    assert(!u->ship); /* you must leave_ship */
    u->ship = sh;
    if (sh && (!sh->_owner || sh->_owner->number <= 0)) {
        ship_set_owner(u);
    }
}

void leave_ship(unit * u)
{
    struct ship *sh = u->ship;

    u->ship = 0;
    if (sh->_owner == u) {
        ship_update_owner(sh);
        sh->_owner = ship_owner(sh);
    }
    set_leftship(u, sh);
}

void leave_building(unit * u)
{
    building * b = u->building;

    u->building = 0;
    if (b->_owner == u) {
        building_update_owner(b);
        assert(b->_owner != u);
    }
}

bool can_leave(unit * u)
{
    static int config;
    static bool rule_leave;

    if (!u->building) {
        return true;
    }
    if (config_changed(&config)) {
        rule_leave = config_get_int("rules.move.owner_leave", 0) != 0;
    }
    if (rule_leave && u->building && u == building_owner(u->building)) {
        return false;
    }
    return true;
}

bool leave(unit * u, bool force)
{
    if (!force) {
        if (!can_leave(u)) {
            return false;
        }
    }
    if (u->building) {
        leave_building(u);
    }
    else if (u->ship) {
        leave_ship(u);
    }
    return true;
}

const struct race *urace(const struct unit *u)
{
    return u->_race;
}

bool can_survive(const unit * u, const region * r)
{
    if ((fval(r->terrain, WALK_INTO) && (u_race(u)->flags & RCF_WALK))
        || (fval(r->terrain, SWIM_INTO) && (u_race(u)->flags & RCF_SWIM))
        || (fval(r->terrain, FLY_INTO) && (u_race(u)->flags & RCF_FLY))) {

        if (has_horses(u) && !fval(r->terrain, WALK_INTO))
            return false;

        if (r->attribs) {
            const curse_type *ctype = ct_find("holyground");
            if (fval(u_race(u), RCF_UNDEAD) && curse_active(get_curse(r->attribs, ctype)))
                return false;
        }
        return true;
    }
    return false;
}

void move_unit(unit * u, region * r, unit ** ulist)
{
    assert(u && r);

    assert(u->faction || !"this unit is dead");
    if (u->region == r)
        return;
    if (!ulist)
        ulist = (&r->units);
    if (u->region) {
        setguard(u, GUARD_NONE);
        fset(u, UFL_MOVED);
        if (u->ship || u->building) {
            /* can_leave must be checked in travel_i */
#ifndef NDEBUG
            bool result = leave(u, false);
            assert(result);
#else
            leave(u, false);
#endif
        }
        translist(&u->region->units, ulist, u);
    }
    else {
        addlist(ulist, u);
    }

    update_interval(u->faction, r);
    u->region = r;
}

/* ist mist, aber wegen nicht skalierender attribute notwendig: */
#include "alchemy.h"

void transfermen(unit * u, unit * dst, int n)
{
    const attrib *a;
    int hp = u->hp;
    region *r = u->region;

    if (n == 0)
        return;
    assert(n > 0);
    /* "hat attackiert"-status wird uebergeben */

    if (dst) {
        skill *sv, *sn;
        skill_t sk;
        ship *sh;

        assert(dst->number + n > 0);

        for (sk = 0; sk != MAXSKILLS; ++sk) {
            int weeks, level = 0;

            sv = unit_skill(u, sk);
            sn = unit_skill(dst, sk);

            if (sv == NULL && sn == NULL)
                continue;
            if (sn == NULL && dst->number == 0) {
                /* new unit, easy to solve */
                level = sv->level;
                weeks = sv->weeks;
            }
            else {
                double dlevel = 0.0;

                if (sv && sv->level) {
                    dlevel += (sv->level + 1 - sv->weeks / (sv->level + 1.0)) * n;
                    level += sv->level * n;
                }
                if (sn && sn->level) {
                    dlevel +=
                        (sn->level + 1 - sn->weeks / (sn->level + 1.0)) * dst->number;
                    level += sn->level * dst->number;
                }

                dlevel = dlevel / (n + dst->number);
                level = level / (n + dst->number);
                if (level <= dlevel) {
                    /* apply the remaining fraction to the number of weeks to go.
                     * subtract the according number of weeks, getting closer to the
                     * next level */
                    level = (int)dlevel;
                    weeks = (level + 1) - (int)((dlevel - level) * (level + 1));
                }
                else {
                    /* make it harder to reach the next level.
                     * weeks+level is the max difficulty, 1 - the fraction between
                     * level and dlevel applied to the number of weeks between this
                     * and the previous level is the added difficutly */
                    level = (int)dlevel + 1;
                    weeks = 1 + 2 * level - (int)((1 + dlevel - level) * level);
                }
            }
            if (level) {
                if (sn == NULL)
                    sn = add_skill(dst, sk);
                sn->level = (unsigned char)level;
                sn->weeks = (unsigned char)weeks;
                assert(sn->weeks > 0 && sn->weeks <= sn->level * 2 + 1);
                assert(dst->number != 0 || (sv && sn->level == sv->level
                    && sn->weeks == sv->weeks));
            }
            else if (sn) {
                remove_skill(dst, sk);
                sn = NULL;
            }
        }
        a = a_find(u->attribs, &at_effect);
        while (a && a->type == &at_effect) {
            effect_data *olde = (effect_data *)a->data.v;
            if (olde->value)
                change_effect(dst, olde->type, olde->value);
            a = a->next;
        }
        sh = leftship(u);
        if (sh != NULL)
            set_leftship(dst, sh);
        dst->flags |=
            u->flags & (UFL_LONGACTION | UFL_NOTMOVING | UFL_HUNGER | UFL_MOVED |
                UFL_ENTER);
        if (u->attribs) {
            transfer_curse(u, dst, n);
        }
    }
    scale_number(u, u->number - n);
    if (dst) {
        set_number(dst, dst->number + n);
        hp -= u->hp;
        dst->hp += hp;
        /* TODO: Das ist schnarchlahm! und gehoert nicht hierhin */
        a = a_find(dst->attribs, &at_effect);
        while (a && a->type == &at_effect) {
            attrib *an = a->next;
            effect_data *olde = (effect_data *)a->data.v;
            int e = get_effect(u, olde->type);
            if (e != 0)
                change_effect(dst, olde->type, -e);
            a = an;
        }
    }
    else if (r->land) {
        if ((u_race(u)->ec_flags & ECF_REC_ETHEREAL) == 0) {
            const race *rc = u_race(u);
            if (rc->ec_flags & ECF_REC_HORSES) {      /* Zentauren an die Pferde */
                int h = rhorses(r) + n;
                rsethorses(r, h);
            }
            else {
                int p = rpeasants(r);
                p += (int)(n * rc->recruit_multi);
                rsetpeasants(r, p);
            }
        }
    }
}

struct building *inside_building(const struct unit *u)
{
    if (!u->building) {
        return NULL;
    }
    else if (!building_finished(u->building)) {
        /* Gebaeude noch nicht fertig */
        return NULL;
    }
    else {
        int p = 0, cap = buildingcapacity(u->building);
        const unit *u2;
        for (u2 = u->region->units; u2; u2 = u2->next) {
            if (u2->building == u->building) {
                p += u2->number;
                if (u2 == u) {
                    if (p <= cap)
                        return u->building;
                    return NULL;
                }
                if (p > cap)
                    return NULL;
            }
        }
    }
    return NULL;
}

void u_setfaction(unit * u, faction * f)
{
    int cnt = u->number;
    if (u->faction == f)
        return;
    if (u->faction) {
        --u->faction->no_units;
        set_number(u, 0);
        join_group(u, NULL);
        free_orders(&u->orders);
        set_order(&u->thisorder, NULL);

        if (u->nextF) {
            u->nextF->prevF = u->prevF;
        }
        if (u->prevF) {
            u->prevF->nextF = u->nextF;
        }
        else {
            u->faction->units = u->nextF;
        }
    }

    if (f != NULL) {
        if (f->units) {
            f->units->prevF = u;
        }
        u->prevF = NULL;
        u->nextF = f->units;
        f->units = u;
    }
    else {
        u->nextF = NULL;
    }

    u->faction = f;
    if (u->region) {
        update_interval(f, u->region);
    }
    if (cnt) {
        set_number(u, cnt);
    }
    if (f) {
        ++f->no_units;
    }
}

void set_number(unit * u, int count)
{
    assert(count >= 0);
    assert(count <= UNIT_MAXSIZE);

    if (count == 0) {
        u->flags &= ~(UFL_HERO);
    }
    if (u->faction) {
        u->faction->num_people += count - u->number;
    }
    u->number = (unsigned short)count;
}

void remove_skill(unit * u, skill_t sk)
{
    skill *sv = u->skills;
    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
        if (sv->id == sk) {
            skill *sl = u->skills + u->skill_size - 1;
            if (sl != sv) {
                *sv = *sl;
            }
            --u->skill_size;
            return;
        }
    }
}

skill *add_skill(unit * u, skill_t id)
{
    skill *sv = u->skills;
#ifndef NDEBUG
    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
        assert(sv->id != id);
    }
#endif
    ++u->skill_size;
    u->skills = realloc(u->skills, u->skill_size * sizeof(skill));
    sv = (u->skills + u->skill_size - 1);
    sv->level = 0;
    sv->weeks = 1;
    sv->old = 0;
    sv->id = id;
    if (id == SK_MAGIC && u->faction && !fval(u->faction, FFL_NPC)) {
        assert(u->number <= 1);
        assert(max_magicians(u->faction) >= u->number);
    }
    return sv;
}

skill *unit_skill(const unit * u, skill_t sk)
{
    skill *sv = u->skills;
    while (sv != u->skills + u->skill_size) {
        if (sv->id == sk)
            return sv;
        ++sv;
    }
    return NULL;
}

bool has_skill(const unit * u, skill_t sk)
{
    skill *sv = u->skills;
    while (sv != u->skills + u->skill_size) {
        if (sv->id == sk) {
            return (sv->level > 0);
        }
        ++sv;
    }
    return false;
}

static int item_invis(const unit *u) {
    const struct resource_type *rring = get_resourcetype(R_RING_OF_INVISIBILITY);
    const struct resource_type *rsphere = get_resourcetype(R_SPHERE_OF_INVISIBILITY);
    return (rring ? i_get(u->items, rring->itype) : 0)
        + (rsphere ? i_get(u->items, rsphere->itype) * 100 : 0);
}

#ifdef NEWATSROI
static int item_modification(const unit * u, skill_t sk, int val)
{
    if (sk == SK_STEALTH) {
        if (item_invis(u) >= u->number) {
            val += ROIBONUS;
        }
    }
    if (sk == SK_PERCEPTION) {
        const struct resource_type *rtype = get_resourcetype(R_AMULET_OF_TRUE_SEEING);
        if (i_get(u->items, rtype->itype) >= u->number) {
            val += ATSBONUS;
        }
    }
    return val;
}
#endif

static int att_modification(const unit * u, skill_t sk)
{
    double result = 0;

    if (u->attribs) {
        curse *c;
        static int cache;
        static const curse_type *skillmod_ct, *worse_ct;
        if (ct_changed(&cache)) {
            skillmod_ct = ct_find("skillmod");
            worse_ct = ct_find("worse");
        }
        c = get_curse(u->attribs, worse_ct);
        if (c != NULL)
            result += curse_geteffect(c);
        if (skillmod_ct) {
            attrib *a = a_find(u->attribs, &at_curse);
            while (a && a->type == &at_curse) {
                curse *c = (curse *)a->data.v;
                if (c->type == skillmod_ct && c->data.i == sk) {
                    result += curse_geteffect(c);
                    break;
                }
                a = a->next;
            }
        }
    }
    /* TODO hier kann nicht mit get/iscursed gearbeitet werden, da nur der
     * jeweils erste vom Typ C_GBDREAM zurueckgegen wird, wir aber alle
     * durchsuchen und aufaddieren muessen */
    if (u->region && u->region->attribs) {
        const curse_type *gbdream_ct = ct_find("gbdream");
        if (gbdream_ct) {
            int bonus = 0, malus = 0;
            attrib *a = a_find(u->region->attribs, &at_curse);
            while (a && a->type == &at_curse) {
                curse *c = (curse *)a->data.v;

                if (curse_active(c) && c->type == gbdream_ct) {
                    int effect = curse_geteffect_int(c);
                    bool allied = alliedunit(c->magician, u->faction, HELP_GUARD);
                    if (allied) {
                        if (effect > bonus) bonus = effect;
                    }
                    else {
                        if (effect < malus) malus = effect;
                    }
                }
                a = a->next;
            }
            result = result + bonus + malus;
        }
    }

    return (int)result;
}

int get_modifier(const unit * u, skill_t sk, int level, const region * r, bool noitem)
{
    int bskill = level;
    int skill = bskill;

    if (r && sk == SK_STEALTH) {
        plane *pl = rplane(r);
        if (pl && fval(pl, PFL_NOSTEALTH)) {
            return 0;
        }
    }

    skill += rc_skillmod(u_race(u), r, sk);
    skill += att_modification(u, sk);

#ifdef NEWATSROI
    if (!noitem) {
        skill = item_modification(u, sk, skill);
    }
#endif
    skill = skillmod(u->attribs, u, r, sk, skill, SMF_ALWAYS);

    if (fval(u, UFL_HUNGER)) {
        if (sk == SK_SAILING && skill > 2) {
            skill = skill - 1;
        }
        else {
            skill = skill / 2;
        }
    }
    return skill - bskill;
}

int eff_skill(const unit * u, const skill *sv, const region *r)
{
    assert(u);
    if (!r) r = u->region;
    if (sv && sv->level > 0) {
        int mlevel = sv->level + get_modifier(u, sv->id, sv->level, r, false);

        if (mlevel > 0) {
            return mlevel;
        }
    }
    return 0;
}

int effskill_study(const unit * u, skill_t sk, const region * r)
{
    skill *sv = unit_skill(u, sk);
    if (sv && sv->level > 0) {
        int mlevel = sv->level;
        if (!r) r = u->region;
        mlevel += get_modifier(u, sv->id, sv->level, r, true);
        if (mlevel > 0)
            return mlevel;
    }
    return 0;
}

int invisible(const unit * target, const unit * viewer)
{
#ifdef NEWATSROI
    return 0;
#else
    if (viewer && viewer->faction == target->faction)
        return 0;
    else {
        int hidden = item_invis(target);
        if (hidden) {
            hidden = _min(hidden, target->number);
            if (viewer) {
                const resource_type *rtype = get_resourcetype(R_AMULET_OF_TRUE_SEEING);
                hidden -= i_get(viewer->items, rtype->itype);
            }
        }
        return hidden;
    }
#endif
}

/** remove the unit from memory.
 * this frees all memory that's only accessible through the unit,
 * and you should already have called uunhash and removed the unit from the
 * region.
 */
void free_unit(unit * u)
{
    free(u->_name);
    free(u->display);
    free_order(u->thisorder);
    free_orders(&u->orders);
    if (u->skills)
        free(u->skills);
    while (u->items) {
        item *it = u->items->next;
        u->items->next = NULL;
        i_free(u->items);
        u->items = it;
    }
    while (u->attribs)
        a_remove(&u->attribs, u->attribs);
    while (u->reservations) {
        struct reservation *res = u->reservations;
        u->reservations = res->next;
        free(res);
    }
}

static int newunitid(void)
{
    int random_unit_no;
    int start_random_no;
    random_unit_no = 1 + (rng_int() % MAX_UNIT_NR);
    start_random_no = random_unit_no;

    while (ufindhash(random_unit_no) || dfindhash(random_unit_no)
        || forbiddenid(random_unit_no)) {
        random_unit_no++;
        if (random_unit_no == MAX_UNIT_NR + 1) {
            random_unit_no = 1;
        }
        if (random_unit_no == start_random_no) {
            random_unit_no = (int)MAX_UNIT_NR + 1;
        }
    }
    return random_unit_no;
}

static void createunitid(unit * u, int id)
{
    if (id <= 0 || id > MAX_UNIT_NR || ufindhash(id) || dfindhash(id)
        || forbiddenid(id))
        u->no = newunitid();
    else
        u->no = id;
    uhash(u);
}

void default_name(const unit *u, char name[], int len) {
    const char * result;
    const struct locale * lang = u->faction ? u->faction->locale : default_locale;
    if (lang) {
        const char * prefix;
        prefix = LOC(lang, "unitdefault");
        if (!prefix) {
            prefix= parameters[P_UNIT];
        }
        result = prefix;
    }
    else {
        result = parameters[P_UNIT];
    }
    strlcpy(name, result, len);
    strlcat(name, " ", len);
    strlcat(name, itoa36(u->no), len);
}

void name_unit(unit * u)
{
    if (u_race(u)->generate_name) {
        char *gen_name = race_namegen(u_race(u), u);
        if (gen_name) {
            free(u->_name);
            u->_name = gen_name;
        }
        else {
            unit_setname(u, racename(u->faction->locale, u, u_race(u)));
        }
    }
    else {
        char name[32];
        default_name(u, name, sizeof(name));
        unit_setname(u, name);
    }
}

/** creates a new unit.
*
* @param dname: name, set to NULL to get a default.
* @param creator: unit to inherit stealth, group, building, ship, etc. from
*/
unit *create_unit(region * r, faction * f, int number, const struct race *urace,
    int id, const char *dname, unit * creator)
{
    unit *u = (unit *)calloc(1, sizeof(unit));

    assert(urace);
    if (f) {
        assert(faction_alive(f));
        u_setfaction(u, f);

        if (f->locale) {
            order *deford = default_order(f->locale);
            if (deford) {
                set_order(&u->thisorder, NULL);
                addlist(&u->orders, deford);
            }
        }
    }
    u_setrace(u, urace);
    u->irace = NULL;

    set_number(u, number);

    /* die nummer der neuen einheit muss vor name_unit generiert werden,
     * da der default name immer noch 'Nummer u->no' ist */
    createunitid(u, id);

    /* zuerst in die Region setzen, da zb Drachennamen den Regionsnamen
     * enthalten */
    if (r)
        move_unit(u, r, NULL);

    /* u->race muss bereits gesetzt sein, wird fuer default-hp gebraucht */
    /* u->region auch */
    u->hp = unit_max_hp(u) * number;

    if (dname) {
        u->_name = _strdup(dname);
    }
    else if (urace->generate_name || playerrace(urace)) {
        name_unit(u);
    }

    if (creator) {
        attrib *a;

        /* erbt Kampfstatus */
        setstatus(u, creator->status);

        /* erbt Gebaeude/Schiff */
        if (creator->region == r) {
            if (creator->building) {
                u_set_building(u, creator->building);
            }
            if (creator->ship && fval(u_race(u), RCF_CANSAIL)) {
                u_set_ship(u, creator->ship);
            }
        }

        /* Tarnlimit wird vererbt */
        if (fval(creator, UFL_STEALTH)) {
            attrib *a = a_find(creator->attribs, &at_stealth);
            if (a) {
                int stealth = a->data.i;
                a = a_add(&u->attribs, a_new(&at_stealth));
                a->data.i = stealth;
            }
        }

        /* Temps von parteigetarnten Einheiten sind wieder parteigetarnt */
        if (fval(creator, UFL_ANON_FACTION)) {
            fset(u, UFL_ANON_FACTION);
        }
        /* Daemonentarnung */
        set_racename(&u->attribs, get_racename(creator->attribs));
        if (fval(u_race(u), RCF_SHAPESHIFT) && fval(u_race(creator), RCF_SHAPESHIFT)) {
            u->irace = creator->irace;
        }

        /* Gruppen */
        if (creator->faction == f && fval(creator, UFL_GROUP)) {
            a = a_find(creator->attribs, &at_group);
            if (a) {
                group *g = (group *)a->data.v;
                set_group(u, g);
            }
        }
        a = a_find(creator->attribs, &at_otherfaction);
        if (a) {
            a_add(&u->attribs, make_otherfaction(get_otherfaction(a)));
        }

        a = a_add(&u->attribs, a_new(&at_creator));
        a->data.v = creator;
    }

    return u;
}

int maxheroes(const struct faction *f)
{
    int nsize = count_all(f);
    if (nsize == 0)
        return 0;
    else {
        int nmax = (int)(log10(nsize / 50.0) * 20);
        return (nmax < 0) ? 0 : nmax;
    }
}

int countheroes(const struct faction *f)
{
    const unit *u = f->units;
    int n = 0;

    while (u) {
        if (fval(u, UFL_HERO))
            n += u->number;
        u = u->nextF;
    }
#ifdef DEBUG_MAXHEROES
    int m = maxheroes(f);
    if (n > m) {
        log_warning("%s has %d of %d heroes\n", factionname(f), n, m);
    }
#endif
    return n;
}

/** Returns the raw unit name, like "Frodo", or "Seeschlange" */
const char *unit_getname(const unit * u)
{
    if (!u->_name) {
        const struct locale * lang = u->faction ? u->faction->locale : default_locale;
        const char *rcname = rc_name_s(u->_race, u->number == 1 ? NAME_SINGULAR : NAME_PLURAL);
        return LOC(lang, rcname);
    }
    return u->_name;
}

void unit_setname(unit * u, const char *name)
{
    free(u->_name);
    if (name && name[0])
        u->_name = _strdup(name);
    else
        u->_name = NULL;
}

const char *unit_getinfo(const unit * u)
{
    return (const char *)u->display;
}

void unit_setinfo(unit * u, const char *info)
{
    free(u->display);
    if (info)
        u->display = _strdup(info);
    else
        u->display = NULL;
}

int unit_getid(const unit * u)
{
    return u->no;
}

void unit_setid(unit * u, int id)
{
    unit *nu = findunit(id);
    if (nu == NULL) {
        uunhash(u);
        u->no = id;
        uhash(u);
    }
}

int unit_gethp(const unit * u)
{
    return u->hp;
}

void unit_sethp(unit * u, int hp)
{
    u->hp = hp;
}

status_t unit_getstatus(const unit * u)
{
    return u->status;
}

void unit_setstatus(unit * u, status_t status)
{
    u->status = status;
}

int unit_getweight(const unit * u)
{
    return weight(u);
}

int unit_getcapacity(const unit * u)
{
    return walkingcapacity(u);
}

void renumber_unit(unit *u, int no) {
    if (no == 0) no = newunitid();
    uunhash(u);
    if (!ualias(u)) {
        attrib *a = a_add(&u->attribs, a_new(&at_alias));
        a->data.i = -u->no;
    }
    u->no = no;
    uhash(u);
}

void unit_addorder(unit * u, order * ord)
{
    order **ordp = &u->orders;
    while (*ordp)
        ordp = &(*ordp)->next;
    *ordp = ord;
    u->faction->lastorders = turn;
}

int unit_max_hp(const unit * u)
{
    int h;
    double p;
    static int config;
    static int rule_stamina;
    h = u_race(u)->hitpoints;

    if (config_changed(&config)) {
        rule_stamina = config_get_int("rules.stamina", STAMINA_AFFECTS_HP);
    }
    if (rule_stamina & 1) {
        p = pow(effskill(u, SK_STAMINA, u->region) / 2.0, 1.5) * 0.2;
        h += (int)(h * p + 0.5);
    }

    /* der healing curse veraendert die maximalen hp */
    if (u->region && u->region->attribs) {
        static int cache;
        static const curse_type *heal_ct;
        if (ct_changed(&cache)) {
            heal_ct = ct_find("healingzone");
        }
        if (heal_ct) {
            curse *c = get_curse(u->region->attribs, heal_ct);
            if (c) {
                h = (int)(h * (1.0 + (curse_geteffect(c) / 100)));
            }
        }
    }
    return h;
}

void scale_number(unit * u, int n)
{
    const attrib *a;
    int remain;

    if (n == u->number)
        return;
    if (n && u->number > 0) {
        int full;
        remain = ((u->hp % u->number) * (n % u->number)) % u->number;

        full = u->hp / u->number;   /* wieviel kriegt jede person mindestens */
        u->hp = full * n + (u->hp - full * u->number) * n / u->number;
        assert(u->hp >= 0);
        if ((rng_int() % u->number) < remain)
            ++u->hp;                  /* Nachkommastellen */
    }
    else {
        remain = 0;
        u->hp = 0;
    }
    if (u->number > 0) {
        for (a = a_find(u->attribs, &at_effect); a && a->type == &at_effect;
        a = a->next) {
            effect_data *data = (effect_data *)a->data.v;
            int snew = data->value / u->number * n;
            if (n) {
                remain = data->value - snew / n * u->number;
                snew += remain * n / u->number;
                remain = (remain * n) % u->number;
                if ((rng_int() % u->number) < remain)
                    ++snew;               /* Nachkommastellen */
            }
            data->value = snew;
        }
    }
    if (u->number == 0 || n == 0) {
        skill_t sk;
        for (sk = 0; sk < MAXSKILLS; sk++) {
            remove_skill(u, sk);
        }
    }

    set_number(u, n);
}

const struct race *u_irace(const struct unit *u)
{
    if (u->irace) {
        return u->irace;
    }
    return u->_race;
}

const struct race *u_race(const struct unit *u)
{
    return u->_race;
}

void u_setrace(struct unit *u, const struct race *rc)
{
    assert(rc);
    u->_race = rc;
}

void unit_add_spell(unit * u, sc_mage * m, struct spell * sp, int level)
{
    sc_mage *mage = m ? m : get_mage(u);

    if (!mage) {
        log_debug("adding new spell %s to a previously non-mage unit %s\n", sp->sname, unitname(u));
        mage = create_mage(u, u->faction ? u->faction->magiegebiet : M_GRAY);
    }
    if (!mage->spellbook) {
        mage->spellbook = create_spellbook(0);
    }
    spellbook_add(mage->spellbook, sp, level);
}

struct spellbook * unit_get_spellbook(const struct unit * u)
{
    sc_mage * mage = get_mage(u);
    if (mage) {
        if (mage->spellbook) {
            return mage->spellbook;
        }
        if (mage->magietyp != M_GRAY) {
            return faction_get_spellbook(u->faction);
        }
    }
    return 0;
}

int effskill(const unit * u, skill_t sk, const region *r)
{
    assert(u);

    if (skill_enabled(sk)) {
        skill *sv = u->skills;
        while (sv != u->skills + u->skill_size) {
            if (sv->id == sk) {
                return eff_skill(u, sv, r);
            }
            ++sv;
        }
    }
    return 0;
}

void remove_empty_units_in_region(region * r)
{
    unit **up = &r->units;

    while (*up) {
        unit *u = *up;

        if (u->number) {
            faction *f = u->faction;
            if (f == NULL || !faction_alive(f)) {
                set_number(u, 0);
            }
        }
        if ((u->number == 0 && u_race(u) != get_race(RC_SPELL)) || (u->age <= 0
            && u_race(u) == get_race(RC_SPELL))) {
            remove_unit(up, u);
        }
        if (*up == u)
            up = &u->next;
    }
}

void remove_empty_units(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        remove_empty_units_in_region(r);
    }
}

typedef char name[OBJECTIDSIZE + 1];
static name idbuf[8];
static int nextbuf = 0;

/** Puts human-readable unit name, with number, like "Frodo (hobb)" into buffer */
char *write_unitname(const unit * u, char *buffer, size_t size)
{
    const char * name = unit_getname(u);
    slprintf(buffer, size, "%s (%s)", name, itoa36(u->no));
    buffer[size - 1] = 0;
    return buffer;
}

/** Returns human-readable unit name, with number, like "Frodo (hobb)" */
const char *unitname(const unit * u)
{
    char *ubuf = idbuf[(++nextbuf) % 8];
    return write_unitname(u, ubuf, sizeof(name));
}

bool unit_name_equals_race(const unit *u) {
    if (u->_name) {
        char sing[32], plur[32];
        const struct locale *lang = u->faction->locale;
        rc_name(u->_race, NAME_SINGULAR, sing, sizeof(sing));
        rc_name(u->_race, NAME_PLURAL, plur, sizeof(plur));
        if (strcmp(u->_name, sing) == 0 || strcmp(u->_name, plur) == 0 ||
            strcmp(u->_name, LOC(lang, sing)) == 0 ||
            strcmp(u->_name, LOC(lang, plur)) == 0) {
            return true;
        }
    }
    return false;
}

bool unit_can_study(const unit *u) {
    return !((u_race(u)->flags & RCF_NOLEARN) || fval(u, UFL_WERE));
}

static int read_newunitid(const faction * f, const region * r)
{
    int n;
    unit *u2;
    n = getid();
    if (n == 0)
        return -1;

    u2 = findnewunit(r, f, n);
    if (u2)
        return u2->no;

    return -1;
}

int read_unitid(const faction * f, const region * r)
{
    char token[16];
    const char *s = gettoken(token, sizeof(token));

    /* Da s nun nur einen string enthaelt, suchen wir ihn direkt in der
    * paramliste. machen wir das nicht, dann wird getnewunit in s nach der
    * nummer suchen, doch dort steht bei temp-units nur "temp" drinnen! */

    if (!s || *s == 0 || !isalnum(*s)) {
        return -1;
    }
    if (isparam(s, f->locale, P_TEMP)) {
        return read_newunitid(f, r);
    }
    return atoi36((const char *)s);
}

int getunit(const region * r, const faction * f, unit **uresult)
{
    unit *u2 = NULL;
    int n = read_unitid(f, r);
    int result = GET_NOTFOUND;

    if (n == 0) {
        result = GET_PEASANTS;
    }
    else if (n > 0) {
        u2 = findunit(n);
        if (u2 != NULL && u2->region == r) {
            /* there used to be a 'u2->flags & UFL_ISNEW || u2->number>0' condition
            * here, but it got removed because of a bug that made units disappear:
            * http://eressea.upb.de/mantis/bug_view_page.php?bug_id=0000172
            */
            result = GET_UNIT;
        }
        else {
            u2 = NULL;
        }
    }
    if (uresult) {
        *uresult = u2;
    }
    return result;
}

int besieged(const unit * u)
{
    /* belagert kann man in schiffen und burgen werden */
    return (u && !keyword_disabled(K_BESIEGE)
        && u->building && u->building->besieged
        && u->building->besieged >= u->building->size * SIEGEFACTOR);
}

bool has_horses(const unit * u)
{
    item *itm = u->items;
    for (; itm; itm = itm->next) {
        if (itm->type->flags & ITF_ANIMAL)
            return true;
    }
    return false;
}

void setstatus(unit *u, int status)
{
    assert(status >= ST_AGGRO && status <= ST_FLEE);
    if (u->status != status) {
        u->status = (status_t)status;
    }
}

#define MAINTENANCE 10
int maintenance_cost(const struct unit *u)
{
    if (u == NULL)
        return MAINTENANCE;
    return u_race(u)->maintenance * u->number;
}

static skill_t limited_skills[] = { SK_MAGIC, SK_ALCHEMY, SK_TACTICS, SK_SPY, SK_HERBALISM, NOSKILL };
bool has_limited_skills(const struct unit * u)
{
    int i, j;

    for (i = 0; i != u->skill_size; ++i) {
        skill *sv = u->skills + i;
        for (j = 0; limited_skills[j] != NOSKILL; ++j) {
            if (sv->id == limited_skills[j]) {
                return true;
            }
        }
    }
    return false;
}

