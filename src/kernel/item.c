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
#include "save.h"
#include "skill.h"
#include "terrain.h"
#include "unit.h"

/* triggers includes */
#include <triggers/changerace.h>
#include <triggers/timeout.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <critbit.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/goodies.h>
#include <util/log.h>
#include <util/language.h>
#include <util/message.h>
#include <util/umlaut.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static critbit_tree inames[MAXLOCALES];
static critbit_tree rnames[MAXLOCALES];
static critbit_tree cb_resources;
luxury_type *luxurytypes;
potion_type *potiontypes;

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
    u->region->land->peasants += delta;
    return u->region->land->peasants;
}

static int golem_factor(const unit *u, const resource_type *rtype) {
    if (rtype == get_resourcetype(R_STONE) && (u_race(u)->flags & RCF_STONEGOLEM)) {
        return GOLEM_STONE;
    }
    if (rtype == get_resourcetype(R_IRON) && (u_race(u)->flags & RCF_IRONGOLEM)) {
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
            static char name[64]; // FIXME: static return value
            _snprintf(name, sizeof(name), "%s_p", rtype->_name);
            return name;
        }
        return rtype->_name;
    }
    return "none";
}

static int num_resources;

static void rt_register(resource_type * rtype)
{
    char buffer[64];
    const char * name = rtype->_name;
    size_t len = strlen(name);

    assert(len < sizeof(buffer) - sizeof(rtype));
    len = cb_new_kv(name, len, &rtype, sizeof(rtype), buffer);
    cb_insert(&cb_resources, buffer, len);
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
            rtype->_name = _strdup(name);
            rt_register(rtype);
        }
    }
    return rtype;
}

static void it_register(item_type * itype)
{
    char buffer[64];
    const char * name = itype->rtype->_name;
    size_t len = strlen(name);

    assert(len < sizeof(buffer) - sizeof(itype));
    len = cb_new_kv(name, len, &itype, sizeof(itype), buffer);
}

static const char *it_aliases[][2] = {
    { "Runenschwert", "runesword" },
    { "p12", "truthpotion" },
    { "p1", "goliathwater" },
    { "p4", "ointment" },
    { "p5", "peasantblood" },
    { "p8", "nestwarmth" },
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
    return zname;
}

item_type *it_find(const char *zname)
{
    const char *name = it_alias(zname);
    resource_type *result = rt_find(name);
    return result ? result->itype : 0;
}

item_type *it_get_or_create(resource_type *rtype) {
    item_type * itype;
    assert(rtype);
    itype = it_find(rtype->_name);
    assert(!itype || !itype->rtype || itype->rtype == rtype);
    if (!itype) {
        itype = (item_type *)calloc(sizeof(item_type), 1);
    }
    itype->rtype = rtype;
    rtype->uchange = res_changeitem;
    rtype->itype = itype;
    rtype->flags |= RTF_ITEM;
    it_register(itype);
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
    int wflags, double magres, const char *damage[], int offmod, int defmod,
    int reload, skill_t sk, int minskill)
{
    weapon_type *wtype;

    assert(minskill > 0);
    assert(resource2weapon(itype->rtype) == NULL);

    wtype = calloc(sizeof(weapon_type), 1);
    if (damage) {
        wtype->damage[0] = _strdup(damage[0]);
        wtype->damage[1] = _strdup(damage[1]);
    }
    wtype->defmod = defmod;
    wtype->flags |= wflags;
    wtype->itype = itype;
    wtype->magres = magres;
    wtype->minskill = minskill;
    wtype->offmod = offmod;
    wtype->reload = reload;
    wtype->skill = sk;
    itype->rtype->wtype = wtype;

    return wtype;
}

armor_type *new_armortype(item_type * itype, double penalty, double magres,
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

static void pt_register(potion_type * ptype)
{
    ptype->itype->rtype->ptype = ptype;
    ptype->next = potiontypes;
    potiontypes = ptype;
}

potion_type *new_potiontype(item_type * itype, int level)
{
    potion_type *ptype;

    assert(resource2potion(itype->rtype) == NULL);

    ptype = (potion_type *)calloc(sizeof(potion_type), 1);
    ptype->itype = itype;
    ptype->level = level;
    pt_register(ptype);

    return ptype;
}

void it_set_appearance(item_type *itype, const char *appearance) {
    assert(itype);
    assert(itype->rtype);
    if (appearance) {
        itype->_appearance[0] = _strdup(appearance);
        itype->_appearance[1] = strcat(strcpy((char *)malloc(strlen((char *)appearance) + 3), (char *)appearance), "_p");
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
#ifdef AT_LTYPE
    attrib *a = a_find(rtype->attribs, &at_ltype);
    if (a)
        return (const luxury_type *)a->data.v;
    return NULL;
#else
    return rtype->ltype;
#endif
}

const potion_type *resource2potion(const resource_type * rtype)
{
#ifdef AT_PTYPE
    attrib *a = a_find(rtype->attribs, &at_ptype);
    if (a)
        return (const potion_type *)a->data.v;
    return NULL;
#else
    return rtype->ptype;
#endif
}

resource_type *rt_find(const char *name)
{
    void * match;
    resource_type *result = 0;

    if (cb_find_prefix(&cb_resources, name, strlen(name) + 1, &match, 1, 0)) {
        cb_get_kv(match, &result, sizeof(result));
    }
    return result;
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
            log_error("serious accounting error. number of items is %d.\n", i->number);
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

#include "region.h"

static int
give_horses(unit * s, unit * d, const item_type * itype, int n,
struct order *ord)
{
    if (d == NULL) {
        int use = use_pooled(s, item2resource(itype), GET_SLACK, n);
        if (use < n)
            use +=
            use_pooled(s, item2resource(itype), GET_RESERVE | GET_POOLED_SLACK,
            n - use);
        rsethorses(s->region, rhorses(s->region) + use);
        return 0;
    }
    return -1;                    /* use the mechanism */
}

static int
give_money(unit * s, unit * d, const item_type * itype, int n,
struct order *ord)
{
    if (d == NULL) {
        int use = use_pooled(s, item2resource(itype), GET_SLACK, n);
        if (use < n)
            use +=
            use_pooled(s, item2resource(itype), GET_RESERVE | GET_POOLED_SLACK,
            n - use);
        rsetmoney(s->region, rmoney(s->region) + use);
        return 0;
    }
    return -1;                    /* use the mechanism */
}

#define R_MINOTHER R_SILVER
#define R_MINHERB R_PLAIN_1
#define R_MINPOTION R_FAST
#define R_MINITEM R_IRON
#define MAXITEMS MAX_ITEMS
#define MAXRESOURCES MAX_RESOURCES
#define MAXHERBS MAX_HERBS
#define MAXPOTIONS MAX_POTIONS
#define MAXHERBSPERPOTION 6

const potion_type *oldpotiontype[MAXPOTIONS + 1];

/*** alte items ***/

static const char *resourcenames[MAX_RESOURCES] = {
    "iron", "stone", "horse", "ao_healing",
    "aots", "roi", "rop", "ao_chastity",
    "laen", "fairyboot", "aoc", "pegasus",
    "elvenhorse", "charger", "dolphin", "roqf", "trollbelt",
    "aurafocus", "sphereofinv", "magicbag",
    "magicherbbag", "dreameye", "p2", "seed", "mallornseed",
    "money", "aura", "permaura",
    "hp", "peasant", "person"
};

const resource_type *get_resourcetype(resource_t type) {
    static int update;
    static struct resource_type * rtypes[MAX_RESOURCES];
    if (update != num_resources) {
        memset(rtypes, 0, sizeof(rtypes));
        update = num_resources;
    }
    const resource_type *rtype = rtypes[type];
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

static int
use_birthdayamulet(unit * u, const struct item_type *itype, int amount,
struct order *ord)
{
    direction_t d;
    message *msg = msg_message("meow", "");

    unused_arg(ord);
    unused_arg(amount);
    unused_arg(itype);

    add_message(&u->region->msgs, msg);
    for (d = 0; d < MAXDIRECTIONS; d++) {
        region *tr = rconnect(u->region, d);
        if (tr)
            add_message(&tr->msgs, msg);
    }
    msg_release(msg);
    return 0;
}

/* t_item::flags */
#define FL_ITEM_CURSED  (1<<0)
#define FL_ITEM_NOTLOST (1<<1)
#define FL_ITEM_NOTINBAG  (1<<2)        /* nicht im Bag Of Holding */
#define FL_ITEM_ANIMAL  (1<<3)  /* ist ein Tier */
#define FL_ITEM_MOUNT ((1<<4) | FL_ITEM_ANIMAL) /* ist ein Reittier */

/* ------------------------------------------------------------- */
/* Kann auch von Nichtmagier benutzt werden, modifiziert Taktik fuer diese
 * Runde um -1 - 4 Punkte. */
static int
use_tacticcrystal(unit * u, const struct item_type *itype, int amount,
struct order *ord)
{
    int i;
    for (i = 0; i != amount; ++i) {
        int duration = 1;           /* wirkt nur eine Runde */
        curse *c;
        float effect;
        float power = 5;            /* Widerstand gegen Antimagiesprueche, ist in diesem
                                       Fall egal, da der curse fuer den Kampf gelten soll,
                                       der vor den Antimagiezaubern passiert */

        effect = (float)(rng_int() % 6 - 1);
        c = create_curse(u, &u->attribs, ct_find("skillmod"), power,
            duration, effect, u->number);
        c->data.i = SK_TACTICS;
        unused_arg(ord);
    }
    use_pooled(u, itype->rtype, GET_DEFAULT, amount);
    ADDMSG(&u->faction->msgs, msg_message("use_tacticcrystal",
        "unit region", u, u->region));
    return 0;
}

typedef struct t_item {
    const char *name;
    /* [0]: Einzahl fuer eigene; [1]: Mehrzahl fuer eigene;
     * [2]: Einzahl fuer Fremde; [3]: Mehrzahl fuer Fremde */
    bool is_resource;
    skill_t skill;
    int minskill;
    int gewicht;
    int preis;
    unsigned int flags;
    void(*benutze_funktion) (struct region *, struct unit *, int amount,
    struct order *);
} t_item;

#include "move.h"

static int
mod_elves_only(const unit * u, const region * r, skill_t sk, int value)
{
    if (u_race(u) == get_race(RC_ELF))
        return value;
    unused_arg(r);
    return -118;
}

static int
mod_dwarves_only(const unit * u, const region * r, skill_t sk, int value)
{
    unused_arg(r);
    if (u_race(u) == get_race(RC_DWARF) || (u_race(u)->flags & RCF_IRONGOLEM)) {
        return value;
    }
    return -118;
}

static int heal(unit * user, int effect)
{
    int req = unit_max_hp(user) * user->number - user->hp;
    if (req > 0) {
        req = _min(req, effect);
        effect -= req;
        user->hp += req;
    }
    return effect;
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

void
register_item_useonother(int(*foo) (struct unit *, int,
const struct item_type *, int, struct order *), const char *name)
{
    register_function((pf_generic)foo, name);
}

static int
use_healingpotion(struct unit *user, const struct item_type *itype, int amount,
struct order *ord)
{
    int effect = amount * 400;
    unit *u = user->region->units;
    effect = heal(user, effect);
    while (effect > 0 && u != NULL) {
        if (u->faction == user->faction) {
            effect = heal(u, effect);
        }
        u = u->next;
    }
    use_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(user, itype->rtype->ptype);

    ADDMSG(&user->faction->msgs, msg_message("usepotion",
        "unit potion", user, itype->rtype));
    return 0;
}

static int
use_warmthpotion(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    if (u->faction->race == get_race(RC_INSECT)) {
        fset(u, UFL_WARMTH);
    }
    else {
        /* nur fuer insekten: */
        cmistake(u, ord, 163, MSG_EVENT);
        return ECUSTOM;
    }
    use_pooled(u, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(u, itype->rtype->ptype);

    ADDMSG(&u->faction->msgs, msg_message("usepotion",
        "unit potion", u, itype->rtype));
    return 0;
}

static int
use_foolpotion(struct unit *u, int targetno, const struct item_type *itype,
int amount, struct order *ord)
{
    unit *target = findunit(targetno);
    if (target == NULL || u->region != target->region) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
            ""));
        return ECUSTOM;
    }
    if (effskill(u, SK_STEALTH, 0) <= effskill(target, SK_PERCEPTION, 0)) {
        cmistake(u, ord, 64, MSG_EVENT);
        return ECUSTOM;
    }
    ADDMSG(&u->faction->msgs, msg_message("givedumb",
        "unit recipient amount", u, target, amount));

    change_effect(target, itype->rtype->ptype, amount);
    use_pooled(u, itype->rtype, GET_DEFAULT, amount);
    return 0;
}

static int
use_bloodpotion(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    if (u->number == 0 || u_race(u) == get_race(RC_DAEMON)) {
        change_effect(u, itype->rtype->ptype, 100 * amount);
    }
    else {
        const race *irace = u_irace(u);
        if (irace == u_race(u)) {
            static const race *rcfailure;
            if (!rcfailure) {
                rcfailure = rc_find("smurf");
                if (!rcfailure)
                    rcfailure = rc_find("toad");
            }
            if (rcfailure) {
                trigger *trestore = trigger_changerace(u, u_race(u), irace);
                if (trestore) {
                    int duration = 2 + rng_int() % 8;

                    add_trigger(&u->attribs, "timer", trigger_timeout(duration,
                        trestore));
                    u->irace = NULL;
                    u_setrace(u, rcfailure);
                }
            }
        }
    }
    use_pooled(u, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        amount);
    usetpotionuse(u, itype->rtype->ptype);

    ADDMSG(&u->faction->msgs, msg_message("usepotion",
        "unit potion", u, itype->rtype));
    return 0;
}

#include <attributes/fleechance.h>
static int
use_mistletoe(struct unit *user, const struct item_type *itype, int amount,
struct order *ord)
{
    int mtoes =
        get_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        user->number);

    if (user->number > mtoes) {
        ADDMSG(&user->faction->msgs, msg_message("use_singleperson",
            "unit item region command", user, itype->rtype, user->region, ord));
        return -1;
    }
    use_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        user->number);
    a_add(&user->attribs, make_fleechance((float)1.0));
    ADDMSG(&user->faction->msgs,
        msg_message("use_item", "unit item", user, itype->rtype));

    return 0;
}

static int
use_magicboost(struct unit *user, const struct item_type *itype, int amount,
struct order *ord)
{
    int mtoes =
        get_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        user->number);
    faction *f = user->faction;
    if (user->number > mtoes) {
        ADDMSG(&user->faction->msgs, msg_message("use_singleperson",
            "unit item region command", user, itype->rtype, user->region, ord));
        return -1;
    }
    if (!is_mage(user) || find_key(f->attribs, atoi36("mbst")) != NULL) {
        cmistake(user, user->thisorder, 214, MSG_EVENT);
        return -1;
    }
    use_pooled(user, itype->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
        user->number);

    a_add(&f->attribs, make_key(atoi36("mbst")));
    set_level(user, findskill("magic"), 3);

    ADDMSG(&user->faction->msgs, msg_message("use_item",
        "unit item", user, itype->rtype));

    return 0;
}

static int
use_snowball(struct unit *user, const struct item_type *itype, int amount,
struct order *ord)
{
    return 0;
}

static void init_oldpotions(void)
{
    const char *potionnames[MAX_POTIONS] = {
        "p0", "goliathwater", "p2", "p3", "ointment", "peasantblood", "p6",
        "p7", "nestwarmth", "p9", "p10", "p11", "truthpotion", "p13", "p14"
    };
    int p;

    for (p = 0; p != MAXPOTIONS; ++p) {
        item_type *itype = it_find(potionnames[p]);
        if (itype != NULL) {
            oldpotiontype[p] = itype->rtype->ptype;
        }
    }
}

void init_resources(void)
{
    resource_type *rtype;

    rtype = rt_get_or_create(resourcenames[R_PERSON]); // lousy hack

    rtype = rt_get_or_create(resourcenames[R_PEASANT]);
    rtype->uchange = res_changepeasants;

    // R_SILVER
    rtype = rt_get_or_create(resourcenames[R_SILVER]);
    rtype->flags |= RTF_ITEM | RTF_POOLED;
    rtype->uchange = res_changeitem;
    rtype->itype = it_get_or_create(rtype);
    rtype->itype->give = give_money;

    // R_PERMAURA
    rtype = rt_get_or_create(resourcenames[R_PERMAURA]);
    rtype->uchange = res_changepermaura;

    // R_LIFE
    rtype = rt_get_or_create(resourcenames[R_LIFE]);
    rtype->uchange = res_changehp;

    // R_AURA
    rtype = rt_get_or_create(resourcenames[R_AURA]);
    rtype->uchange = res_changeaura;

    /* alte typen registrieren: */
    init_oldpotions();
}

int get_money(const unit * u)
{
    const struct resource_type *rtype = get_resourcetype(R_SILVER);
    const item *i = u->items;
    while (i && i->type->rtype != rtype) {
        i = i->next;
    }
    return i ? i->number : 0;
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

static int add_resourcename_cb(const void * match, const void * key, size_t keylen, void *data)
{
    struct locale * lang = (struct locale *)data;
    int i = locale_index(lang);
    critbit_tree * cb = rnames + i;
    resource_type *rtype;

    cb_get_kv(match, &rtype, sizeof(rtype));
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
    return 0;
}

attrib_type at_showitem = {
    "showitem"
};

static int add_itemname_cb(const void * match, const void * key, size_t keylen, void *data)
{
    struct locale * lang = (struct locale *)data;
    critbit_tree * cb = inames + locale_index(lang);
    resource_type *rtype;

    cb_get_kv(match, &rtype, sizeof(rtype));
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

static void init_resourcelimit(attrib * a)
{
    a->data.v = calloc(sizeof(resource_limit), 1);
}

static void finalize_resourcelimit(attrib * a)
{
    free(a->data.v);
}

attrib_type at_resourcelimit = {
    "resourcelimit",
    init_resourcelimit,
    finalize_resourcelimit,
};

static item *default_spoil(const struct race *rc, int size)
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

int free_itype(item_type *itype) {
    free(itype->construction);
    free(itype->_appearance[0]);
    free(itype->_appearance[1]);
    free(itype);
    return 0;
}

int free_rtype_cb(const void * match, const void * key, size_t keylen, void *cbdata) {
    resource_type *rtype;
    cb_get_kv(match, &rtype, sizeof(rtype));
    free(rtype->_name);
    if (rtype->itype) {
        free_itype(rtype->itype);
    }
    free(rtype);
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
    register_function((pf_generic)default_spoil, "defaultdrops");

    register_item_use(use_potion, "usepotion");
    register_item_use(use_potion_delayed, "usepotion_delayed");
    register_item_use(use_tacticcrystal, "use_tacticcrystal");
    register_item_use(use_birthdayamulet, "use_birthdayamulet");
    register_item_use(use_warmthpotion, "usewarmthpotion");
    register_item_use(use_bloodpotion, "usebloodpotion");
    register_item_use(use_healingpotion, "usehealingpotion");
    register_item_useonother(use_foolpotion, "usefoolpotion");
    register_item_use(use_mistletoe, "usemistletoe");
    register_item_use(use_magicboost, "usemagicboost");
    register_item_use(use_snowball, "usesnowball");

    register_item_give(give_horses, "givehorses");
}
