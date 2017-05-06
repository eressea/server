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
#include "ship.h"

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
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/umlaut.h>
#include <util/xml.h>

#include <attributes/movement.h>

#include <storage.h>
#include <selist.h>
#include <critbit.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

selist *shiptypes = NULL;
static critbit_tree cb_shiptypes;

static local_names *snames;

const ship_type *findshiptype(const char *name, const struct locale *lang)
{
    local_names *sn = snames;
    variant var;

    while (sn) {
        if (sn->lang == lang)
            break;
        sn = sn->next;
    }
    if (!sn) {
        selist *ql;
        int qi;

        sn = (local_names *)calloc(sizeof(local_names), 1);
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
    else {
        log_warning("st_find: could not find ship '%s'\n", name);
    }
    return st;
}

const ship_type *st_find(const char *name) {
    return st_find_i(name);
}

ship_type *st_get_or_create(const char * name) {
    ship_type * st = st_find_i(name);
    if (!st) {
        size_t len;
        char data[64];

        st = (ship_type *)calloc(sizeof(ship_type), 1);
        st->_name = strdup(name);
        st->storm = 1.0;
        selist_push(&shiptypes, (void *)st);

        len = cb_new_kv(name, strlen(name), &st, sizeof(st), data);
        assert(len <= sizeof(data));
        cb_insert(&cb_shiptypes, data, len);
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

struct ship *findshipr(const region * r, int n)
{
    ship *sh;

    for (sh = r->ships; sh; sh = sh->next) {
        if (sh->no == n) {
            assert(sh->region == r);
            return sh;
        }
    }
    return 0;
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

    assert(stype);
    sh->no = newcontainerid();
    sh->coast = NODIRECTION;
    sh->type = stype;
    sh->region = r;

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
    slprintf(buffer, sizeof(buffer), "%s %s", sname, itoa36(sh->no));
    sh->name = strdup(buffer);
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
    while (s->attribs)
        a_remove(&s->attribs, s->attribs);
    free(s->name);
    free(s->display);
    free(s);
}

static void free_shiptype(void *ptr) {
    ship_type *stype = (ship_type *)ptr;
    free(stype->_name);
    free(stype->coasts);
    free_construction(stype->construction);
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
    slprintf(ibuf, size, "%s (%s)", sh->name, itoa36(sh->no));
    return ibuf;
}

static int ShipSpeedBonus(const unit * u)
{
    int level = config_get_int("movement.shipspeed.skillbonus", 0);
    if (level > 0) {
        ship *sh = u->ship;
        int skl = effskill(u, SK_SAILING, 0);
        int minsk = (sh->type->cptskill + 1) / 2;
        return (skl - minsk) / level;
    }
    return 0;
}

int crew_skill(const ship *sh) {
    int n = 0;
    unit *u;

    n = 0;

    for (u = sh->region->units; u; u = u->next) {
        if (u->ship == sh) {
            n += effskill(u, SK_SAILING, 0) * u->number;
        }
    }
    return n;
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
    assert(sh->type->construction->improvement == NULL);  /* sonst ist construction::size nicht ship_type::maxsize */

    k = sh->type->range;
    if (sh->size != sh->type->construction->maxsize)
        return 0;

    if (sh->attribs) {
        if (curse_active(get_curse(sh->attribs, ct_find("stormwind")))) {
            k *= 2;
        }
        if (curse_active(get_curse(sh->attribs, ct_find("nodrift")))) {
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
            bonus = MIN(bonus, crew_bonus);
            bonus = MIN(bonus, sh->type->range_max - sh->type->range);
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

    c = get_curse(sh->attribs, ct_find("shipspeedup"));
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
    return write_shipname(sh, ibuf, sizeof(name));
}

int shipcapacity(const ship * sh)
{
    int i = sh->type->cargo;

    /* sonst ist construction:: size nicht ship_type::maxsize */
    assert(!sh->type->construction
        || sh->type->construction->improvement == NULL);

    if (sh->type->construction && sh->size != sh->type->construction->maxsize)
        return 0;

    if (sh->damage) {
        i = (int)ceil(i * (1.0 - sh->damage / sh->size / (double)DAMAGE_SCALE));
    }
    return i;
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

    /* Eigent�mer tot oder kein Eigent�mer vorhanden. Erste lebende Einheit
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
    unit *owner = sh->_owner;
    if (!owner || (owner->ship != sh || owner->number <= 0)) {
        unit * heir = ship_owner_ex(sh, owner ? owner->faction : 0);
        return (heir && heir->number > 0) ? heir : 0;
    }
    return owner;
}

void write_ship_reference(const struct ship *sh, struct storage *store)
{
    WRITE_INT(store, (sh && sh->region) ? sh->no : 0);
}

void ship_setname(ship * self, const char *name)
{
    free(self->name);
    self->name = name ? strdup(name) : 0;
}

const char *ship_getname(const ship * self)
{
    return self->name;
}

int ship_damage_percent(const ship *ship) {
    return (ship->damage * 100 + DAMAGE_SCALE - 1) / (ship->size * DAMAGE_SCALE);
}
