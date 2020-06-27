#ifdef _MSC_VER
# include <platform.h>
#endif
#include <kernel/config.h>
#include "ship.h"

#include <attributes/movement.h>
#include <spells/shipcurse.h>

/* kernel includes */
#include "build.h"
#include "curse.h"
#include "faction.h"
#include "unit.h"
#include "item.h"
#include "race.h"
#include "region.h"
#include "skill.h"

/* util includes */
#include <kernel/attrib.h>
#include <util/base36.h>
#include <kernel/event.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/param.h>
#include <util/strings.h>
#include <util/umlaut.h>

#include <storage.h>
#include <selist.h>
#include <critbit.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

selist *shiptypes = NULL; /* do not use this list for searching*/
static critbit_tree cb_shiptypes; /* use this trie instead */

static local_names *snames;

const ship_type *findshiptype(const char *name, const struct locale *lang)
{
    local_names *sn = snames;
    variant var;

    while (sn && sn->lang != lang) {
        sn = sn->next;
    }
    if (!sn) {
        selist *ql;
        int qi;

        sn = (local_names *)calloc(1, sizeof(local_names));
        if (!sn) abort();
        sn->next = snames;
        sn->lang = lang;

        for (qi = 0, ql = shiptypes; ql; selist_advance(&ql, &qi, 1)) {
            ship_type *stype = (ship_type *)selist_get(ql, qi);
            variant var2;
            const char *n = LOC(lang, stype->_name);
            var2.v = (void *)stype;
            addtoken((struct tnode **)&sn->names, n, var2);
        }
        snames = sn;
    }
    if (findtoken(sn->names, name, &var) == E_TOK_NOMATCH)
        return NULL;
    return (const ship_type *)var.v;
}

static ship_type *st_find_i(const char *name)
{
    const char *match;
    ship_type *st = NULL;

    match = cb_find_str(&cb_shiptypes, name);
    if (match) {
        cb_get_kv(match, &st, sizeof(st));
    }
    return st;
}

const ship_type *st_find(const char *name) {
    ship_type *st = st_find_i(name);
    if (!st) {
        log_warning("st_find: could not find ship '%s'\n", name);
    }
    return st;
}

static void st_register(ship_type *stype) {
    size_t len;
    char data[64];

    selist_push(&shiptypes, (void *)stype);

    len = cb_new_kv(stype->_name, strlen(stype->_name), &stype, sizeof(stype), data);
    assert(len <= sizeof(data));
    cb_insert(&cb_shiptypes, data, len);
}

ship_type *st_get_or_create(const char * name) {
    ship_type * st = st_find_i(name);
    assert(!snames);
    if (!st) {
        st = (ship_type *)calloc(1, sizeof(ship_type));
        if (!st) abort();
        st->_name = str_strdup(name);
        st->storm = 1.0;
        st->tac_bonus = 1.0;
        st_register(st);
    }
    return st;
}

#define MAXSHIPHASH 7919
ship *shiphash[MAXSHIPHASH];
void shash(ship * s)
{
    ship *old = shiphash[s->no % MAXSHIPHASH];

    shiphash[s->no % MAXSHIPHASH] = s;
    s->nexthash = old;
}

void sunhash(ship * s)
{
    ship **show;

    for (show = &shiphash[s->no % MAXSHIPHASH]; *show; show = &(*show)->nexthash) {
        if ((*show)->no == s->no)
            break;
    }
    if (*show) {
        assert(*show == s);
        *show = (*show)->nexthash;
        s->nexthash = 0;
    }
}

static ship *sfindhash(int i)
{
    ship *old;

    for (old = shiphash[i % MAXSHIPHASH]; old; old = old->nexthash)
        if (old->no == i)
            return old;
    return 0;
}

struct ship *findship(int i)
{
    return sfindhash(i);
}

void damage_ship(ship * sh, double percent)
{
    double damage =
        DAMAGE_SCALE * sh->type->damage * percent * sh->size + sh->damage + .000001;
    sh->damage = (int)damage;
}

/* Alte Schiffstypen: */
static ship *deleted_ships;

ship *new_ship(const ship_type * stype, region * r, const struct locale *lang)
{
    static char buffer[32];
    ship *sh = (ship *)calloc(1, sizeof(ship));
    const char *sname = 0;

    if (!sh) abort();
    assert(stype);
    sh->no = newcontainerid();
    sh->coast = NODIRECTION;
    sh->type = stype;
    sh->region = r;
    sh->number = 1;

    if (lang) {
        sname = LOC(lang, stype->_name);
        if (!sname) {
            sname = LOC(lang, parameters[P_SHIP]);
        }
    }
    if (!sname) {
        sname = parameters[P_SHIP];
    }
    assert(sname);
    snprintf(buffer, sizeof(buffer), "%s %s", sname, itoa36(sh->no));
    sh->name = str_strdup(buffer);
    shash(sh);
    if (r) {
        addlist(&r->ships, sh);
    }
    return sh;
}

void remove_ship(ship ** slist, ship * sh)
{
    region *r = sh->region;
    unit *u = r->units;

    handle_event(sh->attribs, "destroy", sh);
    while (u) {
        if (u->ship == sh) {
            leave_ship(u);
        }
        u = u->next;
    }
    sunhash(sh);
    while (*slist && *slist != sh)
        slist = &(*slist)->next;
    assert(*slist);
    *slist = sh->next;
    sh->next = deleted_ships;
    deleted_ships = sh;
    sh->region = NULL;
}

void free_ship(ship * s)
{
    while (s->attribs) {
        a_remove(&s->attribs, s->attribs);
    }
    free(s->name);
    free(s->display);
    free(s);
}

static void free_shiptype(void *ptr) {
    ship_type *stype = (ship_type *)ptr;
    free(stype->_name);
    free(stype->coasts);
    if (stype->construction) {
        free_construction(stype->construction);
    }
    free(stype);
}

void free_shiptypes(void) {
    cb_clear(&cb_shiptypes);
    selist_foreach(shiptypes, free_shiptype);
    selist_free(shiptypes);
    shiptypes = 0;
}

void free_ships(void)
{
    while (deleted_ships) {
        ship *s = deleted_ships;
        deleted_ships = s->next;
    }
}

const char *write_shipname(const ship * sh, char *ibuf, size_t size)
{
    snprintf(ibuf, size, "%s (%s)", sh->name, itoa36(sh->no));
    return ibuf;
}

static int ShipSpeedBonus(const unit * u)
{
    const ship * sh = u->ship;
    static int config;
    static int bonus;

    if (config_changed(&config)) {
        bonus = config_get_int("movement.shipspeed.skillbonus", 0);
    }
    if (bonus > 0) {
        int skl = effskill(u, SK_SAILING, NULL);
        int minsk = (ship_captain_minskill(sh) + 1) / 2;
        return (skl - minsk) / bonus;
    }
    else if (sh->type->flags & SFL_SPEEDY) {
        int base = 3;
        int speed = 0;
        int minsk = ship_captain_minskill(sh) * base;
        int skl = effskill(u, SK_SAILING, NULL);
        while (skl >= minsk) {
            ++speed;
            minsk *= base;
        }
        return speed;
    }
    return 0;
}

int ship_captain_minskill(const ship *sh) {
    return sh->type->cptskill;
}

int shipspeed(const ship * sh, const unit * u)
{
    attrib *a;
    struct curse *c;
    int k, bonus;

    assert(sh);
    if (!u) u = ship_owner(sh);
    if (!u) return 0;
    assert(u->ship == sh);
    assert(u == ship_owner(sh));
    assert(sh->type->construction);

    k = sh->type->range;
    if (!ship_finished(sh)) {
        return 0;
    }
    if (sh->attribs) {
        if (curse_active(get_curse(sh->attribs, &ct_stormwind))) {
            k *= 2;
        }
        if (curse_active(get_curse(sh->attribs, &ct_nodrift))) {
            k += 1;
        }
    }
    if (u->faction->race == u_race(u)) {
        /* race bonus for this faction? */
        if (fval(u_race(u), RCF_SHIPSPEED)) {
            k += 1;
        }
    }

    bonus = ShipSpeedBonus(u);
    if (bonus > 0 && sh->type->range_max > sh->type->range) {
        int crew = crew_skill(sh);
        int crew_bonus = (crew / sh->type->sumskill / 2) - 1;
        if (crew_bonus > 0) {
            int sbonus = sh->type->range_max - sh->type->range;
            if (bonus > sbonus) bonus = sbonus;
            if (bonus > crew_bonus) bonus = crew_bonus;
        }
        else {
            bonus = 0;
        }
    }
    k += bonus;

    a = a_find(sh->attribs, &at_speedup);
    while (a != NULL && a->type == &at_speedup) {
        k += a->data.sa[0];
        a = a->next;
    }

    c = get_curse(sh->attribs, &ct_shipspeedup);
    while (c) {
        k += curse_geteffect_int(c);
        c = c->nexthash;
    }

    if (sh->damage > 0) {
        int size = sh->size * DAMAGE_SCALE;
        k *= (size - sh->damage);
        k = (k + size - 1) / size;
    }
    return k;
}

const char *shipname(const ship * sh)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;
    char *ibuf = idbuf[(++nextbuf) % 8];
    return write_shipname(sh, ibuf, sizeof(idbuf[0]));
}

int ship_maxsize(const ship *sh)
{
    return sh->number * sh->type->construction->maxsize;
}

bool ship_finished(const ship *sh)
{
    if (sh->type->construction) {
        return (sh->size >= ship_maxsize(sh));
    }
    return true;
}

int enoughsailors(const ship * sh, int crew_skill)
{
    return crew_skill >= sh->type->sumskill * sh->number;
}

int crew_skill(const ship *sh) {
    int n = 0;
    unit *u;

    for (u = sh->region->units; u; u = u->next) {
        if (u->ship == sh) {
            int es = effskill(u, SK_SAILING, NULL);
            if (es >= sh->type->minskill) {
                n += es * u->number;
            }
        }
    }
    return n;
}

bool ship_crewed(const ship *sh) {
    unit *u, *cap = ship_owner(sh);
    int capskill = -1, sumskill = 0;
    for (u = sh->region->units; u; u = u->next) {
        if (u->ship == sh) {
            int es = effskill(u, SK_SAILING, NULL);
            if (es > 0) {
                if (u == cap && u->number >= sh->number) {
                    capskill = es;
                }
                if (es >= sh->type->minskill) {
                    sumskill += es * u->number;
                }
            }
        }
    }
    return (capskill >= ship_captain_minskill(sh)) && (sumskill >= sh->type->sumskill * sh->number);
}

void scale_ship(ship *sh, int n)
{
    sh->size = sh->size * n / sh->number;
    sh->damage = sh->damage * n / sh->number;
    sh->number = n;
}

int ship_capacity(const ship * sh)
{
    if (ship_finished(sh)) {
        int i = sh->type->cargo * sh->number;
        if (sh->damage) {
            i = (int)ceil(i * (1.0 - sh->damage / sh->size / (double)DAMAGE_SCALE));
        }
        return i;
    }
    return 0;
}

int ship_cabins(const ship * sh)
{
    if (ship_finished(sh)) {
        int i = sh->type->cabins * sh->number;
        if (sh->damage) {
            i = (int)ceil(i * (1.0 - sh->damage / sh->size / (double)DAMAGE_SCALE));
        }
        return i;
    }
    return 0;
}

void getshipweight(const ship * sh, int *sweight, int *scabins)
{
    unit *u;

    *sweight = 0;
    *scabins = 0;

    for (u = sh->region->units; u; u = u->next) {
        if (u->ship == sh) {
            *sweight += weight(u);
            if (sh->type->cabins) {
                int pweight = u->number * u_race(u)->weight;
                /* weight goes into number of cabins, not cargo */
                *scabins += pweight;
                *sweight -= pweight;
            }
        }
    }
}

void ship_set_owner(unit * u) {
    assert(u && u->ship);
    u->ship->_owner = u;
}

static unit * ship_owner_ex(const ship * sh, const struct faction * last_owner)
{
    unit *u, *heir = 0;

    /* Eigentuemer tot oder kein Eigentuemer vorhanden. Erste lebende Einheit
      * nehmen. */
    for (u = sh->region->units; u; u = u->next) {
        if (u->ship == sh) {
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
    return heir;
}

void ship_update_owner(ship * sh) {
    unit * owner = sh->_owner;
    sh->_owner = ship_owner_ex(sh, owner ? owner->faction : 0);
}

unit *ship_owner(const ship * sh)
{
    if (sh->number > 0) {
        unit *owner = sh->_owner;
        if (!owner || (owner->ship != sh || owner->number <= 0)) {
            unit * heir = ship_owner_ex(sh, owner ? owner->faction : 0);
            return (heir && heir->number > 0) ? heir : 0;
        }
        return owner;
    }
    return NULL;
}

void write_ship_reference(const struct ship *sh, struct storage *store)
{
    WRITE_INT(store, (sh && sh->region) ? sh->no : 0);
}

void ship_setname(ship * sh, const char *name)
{
    free(sh->name);
    sh->name = name ? str_strdup(name) : 0;
}

const char *ship_getname(const ship * sh)
{
    return sh->name;
}

int ship_damage_percent(const ship *sh) {
    /* Schaden muss granularer sein als Gr��e, deshalb ist er skaliert
     * DAMAGE_SCALE ist der Faktor zwischen 1 Schadenspunkt und 1 Gr��enpunkt.
     */
    return ((DAMAGE_SCALE - 1) + sh->damage * 100) / (sh->size * DAMAGE_SCALE);
}
