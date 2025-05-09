#include <kernel/config.h>
#include "unit.h"

#include "ally.h"
#include "attrib.h"
#include "building.h"
#include "calendar.h"
#include "connection.h"
#include "curse.h"
#include "event.h"
#include "faction.h"
#include "gamedata.h"
#include "group.h"
#include "guard.h"
#include "item.h"
#include "move.h"
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "spell.h"
#include "spellbook.h"
#include "ship.h"
#include "skills.h"
#include "terrain.h"
#include "terrainid.h"

#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/stealth.h>

#include <spells/charming.h>
#include <spells/unitcurse.h>
#include <spells/regioncurse.h>

/* util includes */
#include <util/base36.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/variant.h>

#include <storage.h>
#include <strings.h>

#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BAGCAPACITY 20000   /* soviel passt in einen Bag of Holding */

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

#define UMAXHASH 1048573 /* should be prime. 524287 was >90% full */
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
    buddy *friends = NULL;
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
                if (u_race(u2)->ec_flags & ECF_GETITEM) {
                    while (*fr && (*fr)->faction->no < u2->faction->no)
                        fr = &(*fr)->next;
                    nf = *fr;
                    if (nf == NULL || nf->faction != u2->faction) {
                        nf = malloc(sizeof(buddy));
                        assert(nf);
                        nf->next = *fr;
                        nf->faction = u2->faction;
                        nf->unit = u2;
                        nf->number = 0;
                        *fr = nf;
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

    assert(u->region);

    if (u->items == NULL || fval(u_race(u), RCF_ILLUSIONARY))
        return 0;

    /* at first, I should try giving my crap to my own units in this region */
    if (u->faction && (u->faction->flags & FFL_QUIT) == 0 && (flags & GIFT_SELF)) {
        unit *u2, *u3 = NULL;
        for (u2 = r->units; u2; u2 = u2->next) {
            if (u2 != u && u2->faction == u->faction && u2->number > 0) {
                /* some units won't take stuff: */
                if (u_race(u2)->ec_flags & ECF_GETITEM) {
                    i_merge(&u2->items, &u->items);
                    u->items = NULL;
                    break;
                }
                else if (!u3) {
                    /* pick a last-chance recipient: */
                    u3 = u2;
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
    if (NULL != (itm_p = i_find(&u->items, rsilver->itype))) {
        item *itm = *itm_p;
        if (flags & GIFT_PEASANTS) {
            if (u->region->land) {
                rsetmoney(r, rmoney(r) + itm->number);
                itm->number = 0;
            }
        }
        i_remove(&u->items, itm);
        i_free(itm);
    }
    if (NULL != (itm_p = i_find(&u->items, rhorse->itype))) {
        item *itm = *itm_p;
        if (flags & GIFT_PEASANTS) {
            if (u->region->land) {
                rsethorses(r, rhorses(r) + itm->number);
                itm->number = 0;
            }
        }
        i_remove(&u->items, itm);
        i_free(itm);
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

typedef struct dead_faction {
    int key;
    faction* value;
} dead_faction;

static dead_faction* dead_hash;

struct faction *dfindhash(int no) {
    return hmget(dead_hash, no);
}

void erase_unit(unit** ulist, unit* u)
{
    if (u->number) {
        set_number(u, 0);
    }
    leave(u, true);
    u->region = NULL;

    if (u->items) i_freeall(&u->items);

    uunhash(u);
    if (ulist) {
        while (*ulist != u) {
            ulist = &(*ulist)->next;
        }
        assert(*ulist == u);
        *ulist = u->next;
    }

    if (u->faction) {
        hmput(dead_hash, u->no, u->faction);
        if (count_unit(u)) {
            --u->faction->num_units;
        }
        if (u->faction->units == u) {
            u->faction->units = u->nextF;
        }
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

    u->region = NULL;
}

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
    erase_unit(ulist, u);
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

    for (u2 = r->units; u2; u2 = u2->next)
        if (ualias(u2) == n)
            return u2;

    return 0;
}

/* ------------------------------------------------------------- */

/*********************/
/*   at_alias   */
/*********************/
static attrib_type at_alias = {
    "alias",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

/** remember old unit.no (for the creport, mostly)
 * if alias is positive, then this unit was a TEMP
 * if alias is negative, then this unit has been RENUMBERed
 */
int ualias(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_alias);
    if (!a)
        return 0;
    return a->data.i;
}

void usetalias(unit *u, int alias)
{
    a_add(&u->attribs, a_new(&at_alias))->data.i = alias;
}

int a_readprivate(variant *var, void *owner, gamedata *data)
{
    struct storage *store = data->store;
    char lbuf[DISPLAYSIZE];
    READ_STR(store, lbuf, sizeof(lbuf));
    var->v = str_strdup(lbuf);
    return (var->v) ? AT_READ_OK : AT_READ_FAIL;
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
    if (u->display_id > 0) {
        return unit_getinfo(u);
    }
    else {
        char zText[64];
        const char * d;
        const race * rc = u_race(u);
        snprintf(zText, sizeof(zText), "describe_%s", rc->_name);
        d = locale_getstring(lang, zText);
        if (d) {
            return d;
        }
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
    a->data.v = str_strdup(str);
}

/***
** init & cleanup module
**/

void free_units(void)
{
    hmfree(dead_hash);
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

void resolve_unit(unit *u)
{
    resolve(RESOLVE_UNIT | u->no, u);
}

int read_unit_reference(gamedata * data, unit **up, resolve_fun fun)
{
    int id;
    READ_INT(data->store, &id);
    if (up) {
        if (id > 0) {
            *up = findunit(id);
            if (*up == NULL) {
                *up = NULL;
                ur_add(RESOLVE_UNIT | id, (void **)up, fun);
            }
        }
        else {
            *up = NULL;
        }
    }
    return id;
}

unsigned int get_level(const unit * u, enum skill_t id)
{
    assert(id != NOSKILL);
    if (skill_enabled(id)) {
        size_t s, len;
        for (len = arrlen(u->skills), s = 0; s != len; ++s) {
            skill* sv = u->skills + s;
            if (sv->id == id) {
                return sv->level;
            }
            ++sv;
        }
    }
    return 0;
}

void set_level(struct unit * u, enum skill_t sk, unsigned int value)
{
    size_t s, len;

    assert(sk != SK_MAGIC || value==0 || !u->faction || u->number == 1 || fval(u->faction, FFL_NPC));
    assert(value <= UCHAR_MAX);
    if (!skill_enabled(sk))
        return;

    if (value == 0) {
        remove_skill(u, sk);
        return;
    }
    for (len = arrlen(u->skills), s = 0; s != len; ++s) {
        skill* sv = u->skills + s;
        if (sv->id == sk) {
            sk_set_level(sv, value);
            sv->old = sv->level;
            return;
        }
        ++sv;
    }
    sk_set_level(add_skill(u, sk), value);
}

static int leftship_age(struct attrib *a, void *owner)
{
    /* must be aged, so it doesn't affect report generation (cansee) */
    UNUSED_ARG(a);
    UNUSED_ARG(owner);
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

    u->ship = NULL;
    if (sh->_owner == u) {
        ship_update_owner(sh);
        sh->_owner = ship_owner(sh);
    }
    set_leftship(u, sh);
}

void leave_building(unit * u)
{
    building * b = u->building;

    u->building = NULL;
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

bool can_survive(const unit * u, const region * r)
{
    if ((fval(r->terrain, WALK_INTO) && (u_race(u)->flags & RCF_WALK))
        || (fval(r->terrain, SWIM_INTO) && (u_race(u)->flags & RCF_SWIM))
        || (fval(r->terrain, FLY_INTO) && (u_race(u)->flags & RCF_FLY))) {

        if (has_horses(u) && !fval(r->terrain, WALK_INTO))
            return false;

        if (r->attribs) {
            if (fval(u_race(u), RCF_UNDEAD) && curse_active(get_curse(r->attribs, &ct_holyground)))
                return false;
        }
        return true;
    }
    return false;
}

void setguard(unit * u, bool enabled)
{
    if (!enabled) {
        freset(u, UFL_GUARD);
    } else {
        assert(!fval(u, UFL_MOVED));
        assert(u->status < ST_FLEE);
        fset(u, UFL_GUARD);
        fset(u->region, RF_GUARDED);
    }
}

void leave_region(unit* u)
{
    assert(u->region);
    setguard(u, false);
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
}


void move_unit(unit * u, region * r, unit ** ulist)
{
    assert(u && r);

    if (!ulist) {
        ulist = &r->units;
    }
    if (u->region) {
        leave_region(u);
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

void transfermen(unit* u, unit* dst, int n)
{
    region *r = u->region;

    if (n == 0)
        return;
    assert(n > 0);
    assert(u != dst);
    /* "hat attackiert"-status wird uebergeben */

    if (dst) {
        enum skill_t sk;
        ship *sh;
        int delta;

        assert(dst->number + n > 0);

        for (sk = 0; sk != MAXSKILLS; ++sk) {
            int level;
            skill *sv = unit_skill(u, sk);
            skill *sn = unit_skill(dst, sk);
            skill result;
            if (sv == NULL && sn == NULL)
                continue;
            level = merge_skill(sv, sn, &result, dst->number, n);
            if (level > 0) {
                if (sn == NULL) {
                    sn = add_skill(dst, sk);
                }
                sn->level = result.level;
                sn->days = result.days;
            }
            else if (sn) {
                remove_skill(dst, sk);
            }
        }
        sh = leftship(u);
        if (sh != NULL)
            set_leftship(dst, sh);
        dst->flags |=
            u->flags & (UFL_LONGACTION | UFL_NOTMOVING | UFL_HUNGER | UFL_MOVED |
                UFL_ENTER);
        if (u->attribs) {
            transfer_curse(u, dst, n);
            transfer_effects(u, dst, n);
        }
        delta = (uint64_t)u->hp * n / u->number;
        dst->hp += delta;
        u->hp -= delta;
        /* cannot use scale_number here, because it changes hp+effects: */
        set_number(dst, dst->number + n);
        set_number(u, u->number - n);
        if (u->number == 0) {
            remove_skills(u);
        }
        assert(dst->hp >= dst->number);
        assert(u->hp >= u->number);
    }
    else {
        scale_number(u, u->number - n);
        if (r->land) {
            const race* rc = u_race(u);
            if (playerrace(rc) && (rc->ec_flags & ECF_REC_ETHEREAL) == 0) {
                int p = rpeasants(r);
                p += (n / rc->recruit_multi);
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

void u_freeorders(unit *u)
{
    free_orders(&u->orders);
    set_order(&u->thisorder, NULL);
}

void u_setfaction(unit * u, faction * f)
{
    if (u->faction == f)
        return;
    if (u->faction) {
        if (count_unit(u)) {
            --u->faction->num_units;
            u->faction->num_people -= u->number;
        }
        set_group(u, NULL);
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
    if (f && count_unit(u)) {
        ++f->num_units;
        f->num_people += u->number;
    }
}

bool count_unit(const unit *u)
{
    return u_race(u) != NULL;
}

void set_number(unit * u, int count)
{
    assert(count >= 0);
    assert(count <= UNIT_MAXSIZE);

    if (count == 0) {
        u->flags &= ~UFL_HERO;
    }
    if (u->faction && count_unit(u)) {
        u->faction->num_people += count - u->number;
    }
    u->number = count;
}

void remove_skill(unit * u, enum skill_t sk)
{
    ptrdiff_t len = arrlen(u->skills);
    if (len == 1) {
        if (u->skills->id == sk) {
            arrfree(u->skills);
        }
    }
    else if (len > 1) {
        ptrdiff_t s;
        for (s = 0; s != len; ++s) {
            if (u->skills[s].id == sk) {
                arrdel(u->skills, s);
                return;
            }
        }
    }
}

void remove_skills(unit * u) {
    arrfree(u->skills);
}

struct skill *add_skill(struct unit * u, enum skill_t sk)
{
    skill* sv;
    skill skins = { .id = sk, .level = 0, .days = SKILL_DAYS_PER_WEEK, .old = 0 };
    assert(u);
    if (u->skills) {
        ptrdiff_t s, len = arrlen(u->skills);
        for (s = 0; s != len; ++s) {
            sv = u->skills + s;
            if (SK_SKILL(sv) >= sk) break;
        }
        arrins(u->skills, s, skins);
        sv = u->skills + s;
    }
    else {
        arrput(u->skills, skins);
        sv = u->skills;
    }
    if (sk == SK_MAGIC && u->faction && !fval(u->faction, FFL_NPC)) {
        assert(u->number <= 1);
        assert(max_magicians(u->faction) >= u->number);
    }
    return sv;
}

struct skill *unit_skill(const struct unit * u, enum skill_t sk)
{
    assert(u && sk != NOSKILL);

    if (u->skills) {
        ptrdiff_t len = arrlen(u->skills);
        skill* sv = u->skills;
        while (sv != u->skills + len && SK_SKILL(sv) <= sk) {
            if (sv->id == sk) {
                return sv;
            }
            ++sv;
        }
    }
    return NULL;
}

bool has_skill(const unit * u, enum skill_t sk)
{
    ptrdiff_t len = arrlen(u->skills);
    skill *sv = u->skills;
    while (sv != u->skills + len && SK_SKILL(sv) <= sk) {
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

static int att_modification(const unit * u, enum skill_t sk)
{
    double result = 0;

    if (u->attribs) {
        curse *c;
        attrib *a;

        c = get_curse(u->attribs, &ct_worse);
        if (c != NULL) {
            result += curse_geteffect(c);
        }

        a = a_find(u->attribs, &at_curse);
        while (a && a->type == &at_curse) {
            c = (curse *)a->data.v;
            if (c->type == &ct_skillmod && c->data.i == sk) {
                result += curse_geteffect(c);
                break;
            }
            a = a->next;
        }
    }
    /* TODO hier kann nicht mit get/iscursed gearbeitet werden, da nur der
     * jeweils erste vom Typ C_GBDREAM zurueckgegen wird, wir aber alle
     * durchsuchen und aufaddieren muessen */
    if (u->region && u->region->attribs) {
        int bonus = 0, malus = 0;
        attrib *a = a_find(u->region->attribs, &at_curse);
        while (a && a->type == &at_curse) {
            curse *c = (curse *)a->data.v;
            if (c->magician && c->magician->number > 0 && c->type == &ct_gbdream && curse_active(c)) {
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

    return (int)result;
}

int terrain_mod(const race * rc, enum skill_t sk, const region *r)
{
    static int rc_cache, t_cache;
    static const race *rc_dwarf, *rc_insect, *rc_elf;
    static const terrain_type *t_mountain, *t_desert, *t_swamp;
    const struct terrain_type *terrain = r->terrain;

    if (terrain_changed(&t_cache)) {
        t_mountain = get_terrain(terrainnames[T_MOUNTAIN]);
        t_desert = get_terrain(terrainnames[T_DESERT]);
        t_swamp = get_terrain(terrainnames[T_SWAMP]);
    }
    if (rc_changed(&rc_cache)) {
        rc_elf = get_race(RC_ELF);
        rc_dwarf = get_race(RC_DWARF);
        rc_insect = get_race(RC_INSECT);
    }

    if (rc == rc_dwarf) {
        if (sk == SK_TACTICS) {
            if (terrain == t_mountain || fval(terrain, ARCTIC_REGION))
                return 1;
        }
    }
    else if (rc == rc_insect) {
        if (terrain == t_mountain || fval(terrain, ARCTIC_REGION)) {
            return -1;
        }
        else if (terrain == t_desert || terrain == t_swamp) {
            return 1;
        }
    }
    else if (rc == rc_elf) {
        if (r_isforest(r)) {
            if (sk == SK_PERCEPTION || sk == SK_STEALTH) {
                return 1;
            }
            else if (sk == SK_TACTICS) {
                return 2;
            }
        }
    }
    return 0;
}

int get_modifier(const unit * u, enum skill_t sk, int level, const region * r, bool noitem)
{
    const struct race *rc = u_race(u);
    int bskill = level;
    int skill = bskill;

    if (r && sk == SK_STEALTH) {
        plane *pl = rplane(r);
        if (pl && fval(pl, PFL_NOSTEALTH)) {
            return 0;
        }
    }

    skill += rc_skillmod(rc, sk);
    if (skill > 0) {
        if (r) {
            skill += terrain_mod(rc, sk, r);
        }
        skill += att_modification(u, sk);
        if (u->attribs) {
            skill = skillmod(u, r, sk, skill);
        }
    }
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

int effskill_study(const unit * u, enum skill_t sk)
{
    skill *sv = unit_skill(u, sk);
    if (sv && sv->level > 0) {
        int mlevel = sv->level;
        mlevel += get_modifier(u, sv->id, sv->level, u->region, true);
        if (mlevel > 0)
            return mlevel;
    }
    return 0;
}

int invisible(const unit * target, const unit * viewer)
{
    if (viewer && viewer->faction == target->faction)
        return 0;
    else {
        int hidden = item_invis(target);
        if (hidden) {
            if (hidden > target->number) hidden = target->number;
            if (viewer) {
                const resource_type *rtype = get_resourcetype(R_AMULET_OF_TRUE_SEEING);
                if (rtype) {
                    hidden -= i_get(viewer->items, rtype->itype);
                }
            }
        }
        return hidden;
    }
}

/** remove the unit from memory.
 * this frees all memory that's only accessible through the unit,
 * and you should already have called uunhash and removed the unit from the
 * region.
 */
void free_unit(unit * u)
{
    struct reservation **pres = &u->reservations;

    assert(!u->region);
    free(u->_name);
    free_order(u->thisorder);
    free_orders(&u->orders);

    while (*pres) {
        struct reservation *res = *pres;
        *pres = res->next;
        free(res);
    }
    arrfree(u->skills);
    while (u->items) {
        item *it = u->items->next;
        u->items->next = NULL;
        i_free(u->items);
        u->items = it;
    }
    while (u->attribs) {
        a_remove(&u->attribs, u->attribs);
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
            prefix = param_name(P_UNIT, lang);
        }
        result = prefix;
    }
    else {
        result = param_name(P_UNIT, lang);
    }
    snprintf(name, len, "%s %s", result, itoa36(u->no));
}

void name_unit(unit * u)
{
    const race *rc = u_race(u);
    if (rc->name_unit) {
        rc->name_unit(u);
    }
    else if (u->faction->flags & FFL_NPC) {
        unit_setname(u, NULL);
    }
    else {
        char name[32];
        default_name(u, name, sizeof(name));
        unit_setname(u, name);
    }
}

unit *unit_create(int id)
{
    unit *u = (unit *)calloc(1, sizeof(unit));
    createunitid(u, id);
    return u;
}
/** creates a new unit.
*
* @param dname: name, set to NULL to get a default.
* @param creator: unit to inherit stealth, group, building, ship, etc. from
*/
unit *create_unit(region * r, faction * f, int number, const struct race *urace,
    int id, const char *dname, unit * creator)
{
    unit *u = unit_create(id);

    assert(urace);
    u_setrace(u, urace);
    u->irace = NULL;
    if (f) {
        assert(faction_alive(f));
        u_setfaction(u, f);
    }

    set_number(u, number);

    /* zuerst in die Region setzen, da zb Drachennamen den Regionsnamen
     * enthalten */
    if (r)
        move_unit(u, r, NULL);

    /* u->race muss bereits gesetzt sein, wird fuer default-hp gebraucht */
    /* u->region auch */
    if (number > 0) {
        u->hp = unit_max_hp(u) * number;
    }

    if (dname) {
        u->_name = str_strdup(dname);
    }
    else if (urace->name_unit || playerrace(urace)) {
        name_unit(u);
    }

    if (creator) {
        attrib *a;

        /* erbt Kampfstatus und Tarnung */
        unit_setstatus(u, creator->status);
        if (u->_race == creator->_race) {
            u->irace = creator->irace;
        }

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
        if (creator->faction == f) {
            group *g = get_group(creator);
            if (g) {
                set_group(u, g);
            }
        }
        if (creator->attribs) {
            faction *otherf = get_otherfaction(creator);
            if (otherf) {
                set_otherfaction(u, otherf);
            }
        }

        a = a_add(&u->attribs, a_new(&at_creator));
        a->data.v = creator;
    }

    return u;
}

int max_heroes(int num_people)
{
    static int config;
    static int rule_offset;
    if (config_changed(&config)) {
        rule_offset = config_get_int("rules.heroes.offset", 0);
    }
    int nsize = num_people - rule_offset;
    if (nsize <= 0) {
        return 0;
    }
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
        u->_name = str_strdup(name);
    else
        u->_name = NULL;
}

const char *unit_getinfo(const unit * u)
{
    if (u->display_id > 0) {
        return dbstring_load(u->display_id, NULL);
    }
    return NULL;
}

void unit_setinfo(unit * u, const char *info)
{
    if (info) {
        u->display_id = dbstring_save(info);
    }
    else {
        u->display_id = 0;
    }
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

enum status_t unit_getstatus(const unit * u)
{
    return u->status;
}

void unit_setstatus(unit * u, enum status_t status)
{
    u->status = status;
}

int unit_getweight(const unit * u)
{
    return weight(u);
}

int unit_getcapacity(const unit * u)
{
    capacities cap;
    get_transporters(u->items, &cap);
    return walkingcapacity(u, &cap);
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
    static int config;
    static bool rule_stamina;
    h = u_race(u)->hitpoints;

    if (config_changed(&config)) {
        rule_stamina = config_get_int("rules.stamina", 1) != 0;
    }
    if (rule_stamina) {
        double p = pow(effskill(u, SK_STAMINA, u->region) / 2.0, 1.5) * 0.2;
        h += (int)(h * p + 0.5);
    }

    /* der healing curse veraendert die maximalen hp */
    if (u->region && u->region->attribs) {
        curse *c = get_curse(u->region->attribs, &ct_healing);
        if (c) {
            h = (int)(h * (1.0 + (curse_geteffect(c) / 100)));
        }
    }
    return h;
}

void scale_number(unit * u, int n)
{
    if (n == u->number) {
        return;
    }
    if (u->number > 0) {
        if (n > 0) {
            u->hp = (uint64_t)u->hp * n / u->number;
        }
        else {
            a_removeall(&u->attribs, &at_effect);
            u->hp = 0;
        }
    }
    scale_effects(u, n);
    if (u->number == 0 || n == 0) {
        remove_skills(u);
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
    if (u->_race != rc) {
        if (!u->faction) {
            u->_race = rc;
        }
        else {
            int n = 0;
            if (count_unit(u)) {
                --n;
            }
            u->_race = rc;
            if (count_unit(u)) {
                ++n;
            }
            if (n != 0) {
                u->faction->num_units += n;
                u->faction->num_people += n * u->number;
            }
        }
    }
}

int effskill(const unit * u, enum skill_t sk, const region *r)
{
    assert(u);

    if (skill_enabled(sk)) {
        ptrdiff_t len = arrlen(u->skills);
        skill *sv = u->skills;
        while (sv != u->skills + len) {
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
        if (u->number == 0) {
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
    snprintf(buffer, size, "%s (%s)", name, itoa36(u->no));
    buffer[size - 1] = 0;
    return buffer;
}

/** Returns human-readable unit name, with number, like "Frodo (hobb)" */
const char *unitname(const unit * u)
{
    char *ubuf = idbuf[(++nextbuf) % 8];
    return write_unitname(u, ubuf, sizeof(idbuf[0]));
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

static int read_newunitid(const faction * f, const region * r)
{
    int n;
    unit *u2;
    n = getid();
    if (n == 0)
        return -1;

    u2 = findnewunit(r, f, n);
    if (u2 && u2->region == r) {
        return u2->no;
    }

    return -1;
}

int read_unitid(const faction * f, const region * r)
{
    char token[16];
    const char *s = gettoken(token, sizeof(token));

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
        if (u2 != NULL) {
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

bool has_horses(const unit * u)
{
    item *itm = u->items;
    for (; itm; itm = itm->next) {
        if (itm->type->flags & ITF_ANIMAL)
            return true;
    }
    return false;
}

#define MAINTENANCE 10
int maintenance_cost(const struct unit *u)
{
    if (u == NULL) {
        return MAINTENANCE;
    }
    if (IS_PAUSED(u->faction)) {
        return 0;
    }
    return u_race(u)->maintenance * u->number;
}

static skill_t limited_skills[] = { SK_ALCHEMY, SK_HERBALISM, SK_MAGIC, SK_SPY, SK_TACTICS, NOSKILL };

bool is_limited_skill(skill_t sk)
{
    int i;
    for (i = 0; limited_skills[i] != NOSKILL; ++i) {
        if (sk == limited_skills[i]) {
            return true;
        }
    }
    return false;
}

bool has_limited_skills(const struct unit * u)
{
    ptrdiff_t s, len = arrlen(u->skills);

    for (s = 0; s != len; ++s) {
        if (is_limited_skill(u->skills[s].id)) {
            return true;
        }
    }
    return false;
}

bool unit_is_slaved(const unit *u)
{
    return curse_active(get_curse(u->attribs, &ct_slavery));
}

double u_heal_factor(const unit * u)
{
    const race * rc = u_race(u);
    if (rc->healing > 0) {
        return rc->healing / 100.0;
    }
    if (r_isforest(u->region)) {
        static int rc_cache;
        static const race *rc_elf;
        if (rc_changed(&rc_cache)) {
            rc_elf = get_race(RC_ELF);
        }
        if (rc == rc_elf) {
            static int cache;
            static double elf_regen;
            if (config_changed(&cache)) {
                elf_regen = config_get_flt("healing.forest", 1.0);
            }
            return elf_regen;
        }
    }
    return 1.0;
}

void unit_convert_race(unit *u, const race *rc, const char *rcname)
{
    if (rc && u->_race != rc) {
        u_setrace(u, rc);
    }
    if (rcname && strcmp(rcname, u->_race->_name) != 0) {
        set_racename(&u->attribs, rcname);
    }
}

void translate_orders(unit *u, const struct locale *lang, order **list, bool del)
{
    order **po = list;
    (void)lang;
    while (*po) {
        order *ord = *po;
        if (!translate_order(ord, u->faction->locale, lang)) {
            /* we don't know what to do with these, drop or keep them? */
            if (del) {
                if (u->thisorder == ord) {
                    /* FIXME: I don't think this can happen */
                    u->thisorder = NULL;
                }
                *po = ord->next;
                ord->next = NULL;
                free_order(ord);
                continue;
            }
        }
        po = &ord->next;
    }
}

void dump_unit(const unit *u) {
    if (u) {
        const attrib *a;
        fprintf(stdout, "%s, %s, %d %s\n", unitname(u), factionname(u->faction), u->number, u->_race->_name);
        for (a = u->attribs; a; a = a->next) {
            dump_attrib(a);
        }
    }
    else fputs("null", stdout);
}

