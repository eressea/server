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
#include "item.h"
#include "messages.h"
#include "order.h"
#include "parser_helpers.h"
#include "race.h"
#include "region.h"
#include "skill.h"
#include "unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/umlaut.h>
#include <util/parser.h>
#include <quicklist.h>
#include <util/xml.h>

#include <attributes/movement.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

quicklist *shiptypes = NULL;

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
        quicklist *ql;
        int qi;

        sn = (local_names *)calloc(sizeof(local_names), 1);
        sn->next = snames;
        sn->lang = lang;

        for (qi = 0, ql = shiptypes; ql; ql_advance(&ql, &qi, 1)) {
            ship_type *stype = (ship_type *)ql_get(ql, qi);
            variant var2;
            const char *n = LOC(lang, stype->_name);
            var2.v = (void *)stype;
            addtoken(&sn->names, n, var2);
        }
        snames = sn;
    }
    if (findtoken(sn->names, name, &var) == E_TOK_NOMATCH)
        return NULL;
    return (const ship_type *)var.v;
}

static ship_type *st_find_i(const char *name)
{
    quicklist *ql;
    int qi;

    for (qi = 0, ql = shiptypes; ql; ql_advance(&ql, &qi, 1)) {
        ship_type *stype = (ship_type *)ql_get(ql, qi);
        if (strcmp(stype->_name, name) == 0) {
            return stype;
        }
    }
    return NULL;
}

const ship_type *st_find(const char *name) {
    return st_find_i(name);
}

ship_type *st_get_or_create(const char * name) {
    ship_type * st = st_find_i(name);
    if (!st) {
        st = (ship_type *)calloc(sizeof(ship_type), 1);
        st->_name = _strdup(name);
        st->storm = 1.0;
        ql_push(&shiptypes, (void *)st);
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


void fleet_visit(ship *sh, bool_visitor visit_ship, void *state) {
    if (ship_isfleet(sh)) {
        int count = 0;
        ship *shp;
        for (shp = sh->region->ships; shp; ) {
            ship *next = shp->next;
            ++count;
            if (!visit_ship(sh, shp, state))
                break;
            shp = next;
        }
        assert(count > 0);
        visit_ship(sh, NULL, state);
    } else {
        visit_ship(NULL, sh, state);
    }
}

void fleet_const_visit(const ship *sh, const_bool_visitor visit_ship, void *state) {
    if (ship_isfleet(sh)) {
        int count = 0;
        ship *shp;
        for (shp = sh->region->ships; shp;) {
            ship *next = shp->next;
            if (shp->fleet == sh) {
                ++count;
                if (!visit_ship(sh, shp, state))
                    break;
            }
            shp = next;
        }
        assert(count > 0);
        visit_ship(sh, NULL, state);
    } else {
        visit_ship(NULL, sh, state);
    }
}

typedef struct aggregate_state {
    void *inner_state;
    int_visitor visitor;
    aggregator aggregate;
    int value;
} aggregate_state;

static void *aggregate_int_state(int init_value, aggregator agg, int_visitor getvalue, void *inner_state, aggregate_state *state) {
    aggregate_state s2 = { inner_state, getvalue, agg, init_value };
    *state = s2;
    return state;
}

static bool aggregate_int_visitor(const ship *fleet, const ship *sh, void *state) {
    aggregate_state *outer_state = (aggregate_state *) state;
    int value = outer_state->visitor(fleet, sh, outer_state->inner_state);

    outer_state->value = outer_state->aggregate(outer_state->value, value);

    return true;
}

int fleet_const_int_aggregate(const ship *sh, int_visitor getvalue, aggregator aggr, int init_value, void *state) {
    aggregate_state agg_state;
    aggregate_int_state(init_value, aggr, getvalue, state, &agg_state);

    fleet_const_visit(sh, aggregate_int_visitor, &agg_state);
    return agg_state.value;
}

int aggregate_max(int i1, int i2) {
    return _max(i1, i2);
}

int aggregate_min(int i1, int i2) {
    return _min(i1, i2);
}

int aggregate_sum(int i1, int i2) {
    return i1 + i2;
}

static bool damageship(ship *fleet, ship *sh, void *state) {
    double damage;
    if (!sh)
        return true;

    damage = DAMAGE_SCALE * sh->type->damage * *((double *) state) * sh->size + sh->damage + .000001;
    sh->damage = (int)damage;
    return true;
}

void damage_ship(ship * sh, double percent)
{
    double state = percent;

    fleet_visit(sh, damageship, (void *) &state);
}

bool ship_isdamaged(const struct ship *sh) {
    return sh->damage > 0;
}

bool ship_isdestroyed(const struct ship *sh) {
    return sh->damage >= sh->size * DAMAGE_SCALE;
}

/*
int scale_int_damage(int full_value, int damage, int size, int round) {
    full_value - damage / (DAMAGE_SCALE * size) * full_value;asd
}*/

static int applydamage(int speed, int damage, int shsize) {
    int size = shsize * DAMAGE_SCALE;
    speed *= (size - damage);
    speed = (speed + size - 1) / size;
    return speed;
}

static int scale_cargo (const ship *sh, int cargo) {
    return (int)ceil(cargo * (1.0 - sh->damage / sh->size / (double)DAMAGE_SCALE));
}

int ship_damage_percent(const ship *ship) {
    return (ship->damage * 100 + DAMAGE_SCALE - 1) / (ship->size * DAMAGE_SCALE);
}

/* cargo: cargo = (int)ceil(cargo * (1.0 - sh->damage / sh->size / (double)DAMAGE_SCALE));
 * build: (sh->size * DAMAGE_SCALE - sh->damage) / DAMAGE_SCALE;
 */


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
    slprintf(buffer, sizeof(buffer), "%s %s", sname, shipid(sh));
    sh->name = _strdup(buffer);
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
    ql_foreach(shiptypes, free_shiptype);
    ql_free(shiptypes);
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

static int ShipSpeedBonus(const unit * u, const ship *sh)
{
    int level = config_get_int("movement.shipspeed.skillbonus", 0);
    if (level > 0) {
        int skl = effskill(u, SK_SAILING, 0);
        int minsk = (sh->type->cptskill + 1) / 2;
        return (skl - minsk) / level;
    }
    return 0;
}

int ship_crew_skill(const ship *sh) {
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

static const struct curse_type *stormwind_ct, *nodrift_ct;
static bool init;

static int shiprange(const ship *sh) {
    int speed = sh->type->range;
    unit *u = ship_owner(sh);
    attrib *a;
    struct curse *c;

    if (curse_active(get_curse(sh->attribs, stormwind_ct)))
        speed *= 2;
    if (curse_active(get_curse(sh->attribs, nodrift_ct)))
        speed += 1;

    if (u->faction->race == u_race(u)) {
        /* race bonus for this faction? */
        if (fval(u_race(u), RCF_SHIPSPEED)) {
            speed += 1;
        }
    }

    a = a_find(sh->attribs, &at_speedup);
    while (a != NULL && a->type == &at_speedup) {
        speed += a->data.sa[0];
        a = a->next;
    }

    c = get_curse(sh->attribs, ct_find("shipspeedup"));
    while (c) {
        speed += curse_geteffect_int(c);
        c = c->nexthash;
    }

    return speed;
}

typedef struct shipinfo {
    const ship *sh;
    int speed, sumskill, cptbonus;
    int cur_speed, dmg_speed, damage, size;
} shipinfo;

static void assigninfo(shipinfo *info, const ship *shp) {
    info->sh = shp;
    info->speed = shiprange(shp);
    info->sumskill = shp->type->sumskill;
    info->cptbonus = ShipSpeedBonus(ship_owner(shp), shp);
    info->damage = shp->damage;
    info->size = shp->size;
}

static int cmpinfo(const void *i1, const void *i2) {
    shipinfo *info1 = (shipinfo *) i1, *info2 = (shipinfo *) i2;

    return info1->dmg_speed - info2->dmg_speed;
}

static void infosort(shipinfo *ships, int size) {
    qsort(ships, size, sizeof(shipinfo), cmpinfo);
}

int ship_speed(const ship * sh, const unit * u)
{
    int speed;
    int crew;
    ship *shp;
    shipinfo *ships;
    int size = 0, i;

    assert(sh);
    if (!u)
        u = ship_owner(sh);
    if (!u)
        return 0;
    assert(u->ship == sh);
    assert(u == ship_owner(sh));

    if (!init) {
        init = true;
        stormwind_ct = ct_find("stormwind");
        nodrift_ct = ct_find("nodrift");
    }
    if (!ship_iscomplete(sh))
        return 0;

    if (ship_isfleet(sh)) {
        ships = calloc(sh->size, sizeof(shipinfo));
        for (shp = sh->region->ships; shp; shp = shp->next) {
            if (shp->fleet == sh) {
                assigninfo(&ships[size++], shp);
            }
        }
    } else {
        ships = calloc(1, sizeof(shipinfo));
        assigninfo(&ships[size++], sh);
    }

    crew = ship_crew_skill(sh);
    for (i = 0; i < size; ++i) {
        shipinfo *info = &ships[i];
        crew -= info->sumskill;
        if (info->sh->type->range_max <= info->sh->type->range) {
            /* ship does not require larger crew for speedup */
            info->speed += info->cptbonus;
            info->cptbonus = 0;
        }
        info->cur_speed = info->speed;
        info->dmg_speed = applydamage(info->cur_speed, info->damage, info->size);
    }

    infosort(ships, size);
    speed = ships[0].dmg_speed;
    /* assign crew to slower ships first */
    for (i = 0; i < size && crew > 0;) {
        shipinfo *info = &ships[i];
        int dmg_speed;

        assert(info->dmg_speed == speed);
        if (info->cur_speed >= info->sh->type->range_max || info->cur_speed - info->speed >= info->cptbonus) {
            /* maximum speed reached */
            break;
        }
        do {
            /* increase cur_speed by 1; this might not increase dmg_speed */
            crew -= info->sumskill;
            if (crew < 0) {
                break;
            }
            ++info->cur_speed;
            dmg_speed = applydamage(info->cur_speed, info->damage, info->size);
        } while (dmg_speed == info->dmg_speed);
        info->dmg_speed = dmg_speed;
        if (crew < 0)
            break;

        if (i == size - 1 || ships[i + 1].dmg_speed >= info->dmg_speed) {
            ++speed;
            assert(speed == info->dmg_speed);
            i = 0;
        } else {
            assert(speed == info->dmg_speed - 1);
            ++i;
        }
    }

    free(ships);
    return speed;
}

const char *shipname(const ship * sh)
{
    typedef char name[OBJECTIDSIZE + 1];
    static name idbuf[8];
    static int nextbuf = 0;
    char *ibuf = idbuf[(++nextbuf) % 8];
    return write_shipname(sh, ibuf, sizeof(name));
}

static int shpcargo(const ship *fleet, const ship *sh, void *state) {
    int cargo;

    if(sh == NULL)
        return 0;

    cargo = sh->type->cargo;

    if (!ship_iscomplete(sh))
        return 0;

    if (ship_isdamaged(sh)) {
        cargo = scale_cargo(sh, cargo);
    }
    return cargo;
}

int ship_capacity(const ship * sh)
{
    return fleet_const_int_aggregate(sh, shpcargo, aggregate_sum, 0, NULL);
}

static int shpcabins(const ship *fleet, const ship *sh, void *state) {
    if (!sh)
        return 0;
    return sh->type->cabins;
}

int ship_cabins(const ship * sh)
{
    return fleet_const_int_aggregate(sh, shpcabins, aggregate_sum, 0, NULL);
}

void ship_weight(const ship * sh, int *sweight, int *scabins)
{
    unit *u;
    bool cabins = ship_cabins(sh);

    *sweight = 0;
    *scabins = 0;

    for (u = sh->region->units; u; u = u->next) {
        if (u->ship == sh) {
            *sweight += weight(u);
            if (cabins) {
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

    /* Eigentümer tot oder kein Eigentümer vorhanden. Erste lebende Einheit
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
    const ship *fleet = sh->fleet;

    if (fleet) {
        owner = ship_owner(fleet);
    } else {
        fleet = sh;
    }
    if (!owner || (owner->ship != fleet || owner->number <= 0)) {
        unit * heir = ship_owner_ex(fleet, owner ? owner->faction : 0);
        return heir;
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
    self->name = name ? _strdup(name) : 0;
}

const char *ship_name(const ship * self)
{
    return self->name;
}

bool ship_isfleet(const ship *sh) {
    return (sh->type == st_find("fleet"));
}

ship *fleet_add_ship(ship *sh, ship *fleet, unit *cpt) {
    unit * up;
    region *r = sh->region;

    assert(fleet || cpt);
    assert(!sh->fleet);
    if (!fleet) {
        const ship_type *fleet_type = st_find("fleet");
        fleet = new_ship(fleet_type, r, cpt->faction->locale);
    }

    for (up = r->units; up; up = up->next) {
        if (up == cpt || up->ship == sh) {
            up->ship = 0;
            u_set_ship(up, fleet);
        }
    }
    sh->fleet = fleet;
    if (sh->coast != NODIRECTION) {
        assert(fleet->coast == NODIRECTION || fleet->coast == sh->coast);
        fleet->coast = sh->coast;
    }
    ++fleet->size;
    return fleet;
}

ship *fleet_remove_ship(ship *sh, unit *new_cpt) {
    ship *shp, *fleet = sh->fleet;

    sh->fleet = NULL;

    if (new_cpt) {
        new_cpt->ship = 0;
        u_set_ship(new_cpt, sh);
    }

    if (--fleet->size == 0)
        remove_ship(&fleet->region->ships, fleet);
    else if (fleet->coast != NODIRECTION && sh->coast != NODIRECTION) {
        fleet->coast = NODIRECTION;
        for (shp = sh->region->ships; shp; shp = shp->next) {
            if (shp->fleet == fleet) {
                if (shp->coast != NODIRECTION) {
                    fleet->coast = shp->coast;
                    break;
                }
            }
        }
    }

    return fleet->size > 0 ? fleet : NULL;
}

void fleet_cmd(region * r)
{
    unit **uptr;
    ship *sh;

    for (uptr = &r->units; *uptr;) {
        unit *u = *uptr;
        order **ordp = &u->orders;

        while (*ordp) {
            order *ord = *ordp;
            if (getkeyword(ord) == K_FLEET) {
                char token[128];
                const char *s;
                param_t p;
                int id, id2;
                unit *cpt = NULL;

                init_order(ord);
                s = gettoken(token, sizeof(token));

                p = s?findparam(s, u->faction->locale):NOPARAM;
                if (p == P_REMOVE) {
                    id = getid();
                    if (id == 0) {
                        /* oops, maybe we mistakenly took a ship number for a token? */
                        id = atoi36(s);
                    } else {
                        sh = findship(id);
                        if (sh && sh->region != u->region) {
                            sh = NULL;
                        }
                        if (sh) {
                            id2 = getid();
                            if (id2 != 0) {
                                cpt = findunit(id2);
                                if (!cpt || cpt->region != u->region) {
                                    ADDMSG(&u->faction->msgs,
                                        msg_feedback(u, ord, "feedback_unit_not_found", ""));
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    id = s?atoi36(s):0;
                }

                if (p != P_REMOVE) {
                    /* add ship */
                    if (id) {
                        sh = findship(id);
                    } else {
                        sh = u->ship;
                        if (ship_isfleet(sh)) {
                            /* ignore FLOTTE with no params if already own a fleet */
                            continue;
                        }
                    }

                    if (!(u_race(u)->flags & (RCF_CANSAIL | RCF_WALK | RCF_FLY))) {
                        cmistake(u, ord, 233, MSG_MOVE);
                        break;
                    }
                    if (!sh || sh->fleet || ship_isfleet(sh) || sh->region != u->region) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "fleet_ship_invalid", "ship", sh));
                        break;
                    }
                    if (ship_owner(sh) && !ucontact(ship_owner(sh), u)) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact", "target", ship_owner(sh)));
                        break;
                    }

                    if (u->ship && ship_isfleet(u->ship)) {
                        if (sh->coast != NODIRECTION && u->ship->coast != NODIRECTION && sh->coast != u->ship->coast){
                            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "fleet_ship_invalid", "ship", sh));
                            break;
                        }
                        fleet_add_ship(sh, u->ship, u);
                    } else {
                        fleet_add_ship(sh, NULL, u);
                    }

                } else {
                    /* remove ship */
                    if (!u->ship || !ship_isfleet(u->ship) || u != ship_owner(u->ship)) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "fleet_only_captain", "id", id));
                        break;
                    }
                    if (!sh || sh->fleet != u->ship || u != ship_owner(sh)) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "ship_nofleet", "id", id));
                        break;
                    }
                    if (id2 != 0 && !cpt) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "unitnotfound_id", "id", id));
                        break;
                    }
                    if (cpt && !(u_race(cpt)->flags & (RCF_CANSAIL | RCF_WALK | RCF_FLY))) {
                        cmistake(u, ord, 233, MSG_MOVE);
                        cmistake(cpt, ord, 233, MSG_MOVE);
                        cpt = NULL;
                    }
                    if (cpt && !ucontact(cpt, u)) {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact", "target", cpt));
                        break;
                    }

                    fleet_remove_ship(sh, cpt);
                }
            }
            if (*ordp == ord)
                ordp = &ord->next;
        }
        if (*uptr == u)
            uptr = &u->next;
    }
}

static int cptskill(const ship *fleet, const ship *sh, void *state) {
    if (!sh)
        return fleet->type->cptskill;
    return sh->type->cptskill;
}

int ship_type_cpt_skill(const ship *sh) {
    return fleet_const_int_aggregate(sh, cptskill, aggregate_max, INT_MIN, NULL);
}

static int shipcmplt(const ship *fleet, const ship *sh, void *state) {
    if (!sh)
        return 1;
    if (sh->type->construction) {
        assert(!sh->type->construction->improvement); /* sonst ist construction::size nicht ship_type::maxsize */
        if (sh->size != sh->type->construction->maxsize) {
            return 0;
        }
    }
    return 1;
}

bool ship_iscomplete(const ship *sh) {
    return fleet_const_int_aggregate(sh, shipcmplt, aggregate_min, 1, NULL) == 1;
}

static int sumskill(const ship *fleet, const ship *sh, void *state) {
    if (!sh)
        return 0;
    return sh->type->sumskill;
}

int ship_type_crew_skill(const ship *sh) {
    return fleet_const_int_aggregate(sh, sumskill, aggregate_sum, 0, NULL);
}
