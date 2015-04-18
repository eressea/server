/*
Copyright (c) 1998-2014,
Enno Rehling <enno@eressea.de>
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
#include "move.h"
#include "laws.h"
#include "reports.h"
#include "alchemy.h"
#include "vortex.h"
#include "monster.h"
#include "lighthouse.h"

#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/connection.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/render.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include "direction.h"
#include "skill.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/rng.h>

#include <storage.h>

/* attributes includes */
#include <attributes/follow.h>
#include <attributes/movement.h>
#include <attributes/otherfaction.h>
#include <attributes/stealth.h>
#include <attributes/targetregion.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *storms;

typedef struct traveldir {
    int no;
    direction_t dir;
    int age;
} traveldir;

static attrib_type at_traveldir = {
    "traveldir",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,                  /* Weil normales Aging an ungünstiger Stelle */
    a_writechars,
    a_readchars
};

typedef struct follower {
    struct follower *next;
    unit *uf;
    unit *ut;
    const region_list *route_end;
} follower;

static void
get_followers(unit * target, region * r, const region_list * route_end,
follower ** followers)
{
    unit *uf;
    for (uf = r->units; uf; uf = uf->next) {
        if (fval(uf, UFL_FOLLOWING) && !fval(uf, UFL_NOTMOVING)) {
            const attrib *a = a_findc(uf->attribs, &at_follow);
            if (a && a->data.v == target) {
                follower *fnew = (follower *)malloc(sizeof(follower));
                fnew->uf = uf;
                fnew->ut = target;
                fnew->route_end = route_end;
                fnew->next = *followers;
                *followers = fnew;
            }
        }
    }
}

static void shiptrail_init(attrib * a)
{
    a->data.v = calloc(1, sizeof(traveldir));
}

static void shiptrail_finalize(attrib * a)
{
    free(a->data.v);
}

static int shiptrail_age(attrib * a)
{
    traveldir *t = (traveldir *)(a->data.v);

    t->age--;
    return (t->age > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

static int shiptrail_read(attrib * a, void *owner, struct storage *store)
{
    int n;
    traveldir *t = (traveldir *)(a->data.v);

    READ_INT(store, &t->no);
    READ_INT(store, &n);
    t->dir = (direction_t)n;
    READ_INT(store, &t->age);
    return AT_READ_OK;
}

static void
shiptrail_write(const attrib * a, const void *owner, struct storage *store)
{
    traveldir *t = (traveldir *)(a->data.v);
    WRITE_INT(store, t->no);
    WRITE_INT(store, t->dir);
    WRITE_INT(store, t->age);
}

attrib_type at_shiptrail = {
    "traveldir_new",
    shiptrail_init,
    shiptrail_finalize,
    shiptrail_age,
    shiptrail_write,
    shiptrail_read
};

/* ------------------------------------------------------------- */

static attrib_type at_driveweight = {
    "driveweight", NULL, NULL, NULL, NULL, NULL
};

static bool entrance_allowed(const struct unit *u, const struct region *r)
{
#ifdef REGIONOWNERS
    faction *owner = region_get_owner(r);
    if (owner == NULL || u->faction == owner)
        return true;
    if (alliedfaction(rplane(r), owner, u->faction, HELP_TRAVEL))
        return true;
    return false;
#else
    return true;
#endif
}

int personcapacity(const unit * u)
{
    int cap = u_race(u)->weight + u_race(u)->capacity;
    return cap;
}

static int eff_weight(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_driveweight);

    if (a)
        return weight(u) + a->data.i;

    return weight(u);
}

static void
get_transporters(const item * itm, int *p_animals, int *p_acap, int *p_vehicles,
int *p_vcap)
{
    int vehicles = 0, vcap = 0;
    int animals = 0, acap = 0;

    for (; itm != NULL; itm = itm->next) {
        const item_type *itype = itm->type;
        if (itype->capacity > 0) {
            if (itype->flags & ITF_ANIMAL) {
                animals += itm->number;
                if (acap == 0)
                    acap = itype->capacity;
                assert(acap == itype->capacity
                    || !"animals with different capacity not supported");
            }
            if (itype->flags & ITF_VEHICLE) {
                vehicles += itm->number;
                if (vcap == 0)
                    vcap = itype->capacity;
                assert(vcap == itype->capacity
                    || !"vehicles with different capacity not supported");
            }
        }
    }
    *p_vehicles = vehicles;
    *p_animals = animals;
    *p_vcap = vcap;
    *p_acap = acap;
}

static int ridingcapacity(unit * u)
{
    int vehicles = 0, vcap = 0;
    int animals = 0, acap = 0;

    get_transporters(u->items, &animals, &acap, &vehicles, &vcap);

    /* Man trägt sein eigenes Gewicht plus seine Kapazität! Die Menschen
     ** tragen nichts (siehe walkingcapacity). Ein Wagen zählt nur, wenn er
     ** von zwei Pferden gezogen wird */

    animals = _min(animals, effskill(u, SK_RIDING) * u->number * 2);
    if (fval(u_race(u), RCF_HORSE))
        animals += u->number;

    /* maximal diese Pferde können zum Ziehen benutzt werden */
    vehicles = _min(animals / HORSESNEEDED, vehicles);

    return vehicles * vcap + animals * acap;
}

int walkingcapacity(const struct unit *u)
{
    int n, people, pferde_fuer_wagen;
    int wagen_ohne_pferde, wagen_mit_pferden, wagen_mit_trollen;
    int vehicles = 0, vcap = 0;
    int animals = 0, acap = 0;
    const struct resource_type *rhorse = rt_find("horse");
    const struct resource_type *rbelt = rt_find("trollbelt");

    get_transporters(u->items, &animals, &acap, &vehicles, &vcap);

    /* Das Gewicht, welches die Pferde tragen, plus das Gewicht, welches
     * die Leute tragen */

    pferde_fuer_wagen = _min(animals, effskill(u, SK_RIDING) * u->number * 4);
    if (fval(u_race(u), RCF_HORSE)) {
        animals += u->number;
        people = 0;
    }
    else {
        people = u->number;
    }

    /* maximal diese Pferde können zum Ziehen benutzt werden */
    wagen_mit_pferden = _min(vehicles, pferde_fuer_wagen / HORSESNEEDED);

    n = wagen_mit_pferden * vcap;

    if (u_race(u) == get_race(RC_TROLL)) {
        /* 4 Trolle ziehen einen Wagen. */
        /* Unbesetzte Wagen feststellen */
        wagen_ohne_pferde = vehicles - wagen_mit_pferden;

        /* Genug Trolle, um die Restwagen zu ziehen? */
        wagen_mit_trollen = _min(u->number / 4, wagen_ohne_pferde);

        /* Wagenkapazität hinzuzählen */
        n += wagen_mit_trollen * vcap;
        wagen_ohne_pferde -= wagen_mit_trollen;
    }

    n += animals * acap;
    n += people * personcapacity(u);
    /* Goliathwasser */
    if (rhorse) {
        int tmp = get_effect(u, oldpotiontype[P_STRONG]);
        if (tmp > 0) {
            int horsecap = rhorse->itype->capacity;
            if (tmp > people) {
                tmp = people;
            }
            n += tmp * (horsecap - personcapacity(u));
        }
    }
    if (rbelt) {
        int belts = i_get(u->items, rbelt->itype);
        if (belts) {
            int multi = get_param_int(global.parameters, "rules.trollbelt.multiplier", STRENGTHMULTIPLIER);
            n += _min(people, belts) * (multi - 1) * u_race(u)->capacity;
        }
    }

    return n;
}

enum {
    E_CANWALK_OK = 0,
    E_CANWALK_TOOMANYHORSES,
    E_CANWALK_TOOMANYCARTS,
    E_CANWALK_TOOHEAVY
};

static int canwalk(unit * u)
{
    int maxwagen, maxpferde;
    int vehicles = 0, vcap = 0;
    int animals = 0, acap = 0;

    /* workaround: monsters are too stupid to drop items, therefore they have
     * infinite carrying capacity */

    if (is_monsters(u->faction))
        return E_CANWALK_OK;

    get_transporters(u->items, &animals, &acap, &vehicles, &vcap);

    maxwagen = effskill(u, SK_RIDING) * u->number * 2;
    if (u_race(u) == get_race(RC_TROLL)) {
        maxwagen = _max(maxwagen, u->number / 4);
    }
    maxpferde = effskill(u, SK_RIDING) * u->number * 4 + u->number;

    if (animals > maxpferde)
        return E_CANWALK_TOOMANYHORSES;

    if (walkingcapacity(u) - eff_weight(u) >= 0)
        return E_CANWALK_OK;

    /* Stimmt das Gewicht, impliziert dies hier, daß alle Wagen ohne
     * Zugpferde/-trolle als Fracht aufgeladen wurden: zu viele Pferde hat
     * die Einheit nicht zum Ziehen benutzt, also nicht mehr Wagen gezogen
     * als erlaubt. */

    if (vehicles > maxwagen)
        return E_CANWALK_TOOMANYCARTS;
    /* Es muß nicht zwingend an den Wagen liegen, aber egal... (man
     * könnte z.B. auch 8 Eisen abladen, damit ein weiterer Wagen als
     * Fracht draufpaßt) */

    return E_CANWALK_TOOHEAVY;
}

bool canfly(unit * u)
{
    if (i_get(u->items, it_find("pegasus")) >= u->number && effskill(u, SK_RIDING) >= 4)
        return true;

    if (fval(u_race(u), RCF_FLY))
        return true;

    if (get_movement(&u->attribs, MV_FLY))
        return true;

    return false;
}

bool canswim(unit * u)
{
    if (i_get(u->items, it_find("dolphin")) >= u->number && effskill(u, SK_RIDING) >= 4)
        return true;

    if (u_race(u)->flags & RCF_FLY)
        return true;

    if (u_race(u)->flags & RCF_SWIM)
        return true;

    if (get_movement(&u->attribs, MV_FLY))
        return true;

    if (get_movement(&u->attribs, MV_SWIM))
        return true;

    return false;
}

static int canride(unit * u)
{
    int horses = 0, maxhorses, unicorns = 0, maxunicorns;
    int skill = effskill(u, SK_RIDING);
    item *itm;
    const item_type *it_horse, *it_elvenhorse, *it_charger;
    const resource_type *rtype;

    it_horse = ((rtype = get_resourcetype(R_HORSE)) != NULL) ? rtype->itype : 0;
    it_elvenhorse = ((rtype = get_resourcetype(R_UNICORN)) != NULL) ? rtype->itype : 0;
    it_charger = ((rtype = get_resourcetype(R_CHARGER)) != NULL) ? rtype->itype : 0;

    for (itm = u->items; itm; itm = itm->next) {
        if (itm->type == it_horse || itm->type == it_charger) {
            horses += itm->number;
        }
        else if (itm->type == it_elvenhorse) {
            unicorns += itm->number;
        }
    }

    maxunicorns = (skill / 5) * u->number;
    maxhorses = skill * u->number * 2;

    if (!(u_race(u)->flags & RCF_HORSE)
        && ((horses == 0 && unicorns == 0)
        || horses > maxhorses || unicorns > maxunicorns)) {
        return 0;
    }

    if (ridingcapacity(u) - eff_weight(u) >= 0) {
        if (horses == 0 && unicorns >= u->number && !(u_race(u)->flags & RCF_HORSE)) {
            return 2;
        }
        return 1;
    }

    return 0;
}

static bool cansail(const region * r, ship * sh)
{
    /* sonst ist construction:: size nicht ship_type::maxsize */
    assert(!sh->type->construction
        || sh->type->construction->improvement == NULL);

    if (sh->type->construction && sh->size != sh->type->construction->maxsize) {
        return false;
    }
    else {
        int n = 0, p = 0;
        int mweight = shipcapacity(sh);
        int mcabins = sh->type->cabins;

        getshipweight(sh, &n, &p);

        if (n > mweight)
            return false;
        if (mcabins && p > mcabins)
            return false;
    }
    return true;
}

int enoughsailors(const ship * sh, int crew_skill)
{
    return crew_skill >= sh->type->sumskill;
}

/* ------------------------------------------------------------- */

static ship *do_maelstrom(region * r, unit * u)
{
    int damage;
    ship *sh = u->ship;

    damage = rng_int() % 75 + rng_int() % 75 - eff_skill(u, SK_SAILING, r) * 4;

    if (damage <= 0) {
        return sh;
    }

    damage_ship(u->ship, 0.01 * damage);

    if (sh->damage >= sh->size * DAMAGE_SCALE) {
        ADDMSG(&u->faction->msgs, msg_message("entermaelstrom",
            "region ship damage sink", r, sh, damage, 1));
        remove_ship(&sh->region->ships, sh);
        return NULL;
    }
    ADDMSG(&u->faction->msgs, msg_message("entermaelstrom",
        "region ship damage sink", r, sh, damage, 0));
    return u->ship;
}

/** sets a marker in the region telling that the unit has travelled through it
 * this is used for two distinctly different purposes:
 * - to report that a unit has travelled through. the report function
 *   makes sure to only report the ships of travellers, not the travellers
 *   themselves
 * - to report the region to the traveller
 */
void travelthru(const unit * u, region * r)
{
    attrib *ru = a_add(&r->attribs, a_new(&at_travelunit));

    fset(r, RF_TRAVELUNIT);

    ru->data.v = (void *)u;

    /* the first and last region of the faction gets reset, because travelthrough
     * could be in regions that are located before the [first, last] interval,
     * and recalculation is needed */
#ifdef SMART_INTERVALS
    update_interval(u->faction, r);
#endif
}

static direction_t
koor_reldirection(int ax, int ay, int bx, int by, const struct plane *pl)
{
    int dir;
    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
        int x = ax + delta_x[dir];
        int y = ay + delta_y[dir];
        pnormalize(&x, &y, pl);
        if (bx == x && by == y)
            return (direction_t)dir;
    }
    return NODIRECTION;
}

direction_t reldirection(const region * from, const region * to)
{
    plane *pl = rplane(from);
    if (pl == rplane(to)) {
        direction_t dir = koor_reldirection(from->x, from->y, to->x, to->y, pl);

        if (dir == NODIRECTION) {
            spec_direction *sd = special_direction(from, to);
            if (sd != NULL && sd->active)
                return D_SPECIAL;
        }
        return dir;
    }
    return NODIRECTION;
}

static void leave_trail(ship * sh, region * from, region_list * route)
{
    region *r = from;

    while (route != NULL) {
        region *rn = route->data;
        direction_t dir = reldirection(r, rn);

        /* TODO: we cannot leave a trail into special directions
         * if we use this kind of direction-attribute */
        if (dir < MAXDIRECTIONS && dir >= 0) {
            traveldir *td = NULL;
            attrib *a = a_find(r->attribs, &at_shiptrail);

            while (a != NULL && a->type == &at_shiptrail) {
                td = (traveldir *)a->data.v;
                if (td->no == sh->no)
                    break;
                a = a->next;
            }

            if (a == NULL || a->type != &at_shiptrail) {
                a = a_add(&(r->attribs), a_new(&at_shiptrail));
                td = (traveldir *)a->data.v;
                td->no = sh->no;
            }
            td->dir = dir;
            td->age = 2;
        }
        route = route->next;
        r = rn;
    }
}

static void
mark_travelthru(const unit * u, region * r, const region_list * route,
const region_list * route_end)
{
    /* kein travelthru in der letzten region! */
    while (route != route_end) {
        travelthru(u, r);
        r = route->data;
        route = route->next;
    }
}

ship *move_ship(ship * sh, region * from, region * to, region_list * route)
{
    unit **iunit = &from->units;
    unit **ulist = &to->units;
    bool trail = (route == NULL);

    if (from != to) {
        translist(&from->ships, &to->ships, sh);
        sh->region = to;
    }
    if (!trail) {
        leave_trail(sh, from, route);
        trail = true;
    }

    while (*iunit != NULL) {
        unit *u = *iunit;
        assert(u->region == from);

        if (u->ship == sh) {
            if (route != NULL)
                mark_travelthru(u, from, route, NULL);
            if (from != to) {
                u->ship = 0;  /* temporary trick -- do not use u_set_ship here */
                move_unit(u, to, ulist);
                ulist = &u->next;
                u->ship = sh; /* undo the trick -- do not use u_set_ship here */
            }
            if (route && eff_skill(u, SK_SAILING, from) >= 1) {
                produceexp(u, SK_SAILING, u->number);
            }
        }
        if (*iunit == u)
            iunit = &u->next;
    }

    return sh;
}

static bool is_freezing(const unit * u)
{
    if (u_race(u) != get_race(RC_INSECT))
        return false;
    if (is_cursed(u->attribs, C_KAELTESCHUTZ, 0))
        return false;
    return true;
}

int check_ship_allowed(struct ship *sh, const region * r)
{
    int c = 0;
    const building_type *bt_harbour = NULL;
    bt_harbour = bt_find("harbour");

    if (sh->region && r_insectstalled(r)) {
        /* insekten dürfen nicht hier rein. haben wir welche? */
        unit *u;

        for (u = sh->region->units; u != NULL; u = u->next) {
            if (u->ship != sh) {
                continue;
            }

            if (is_freezing(u)) {
                unit *captain = ship_owner(sh);
                if (captain) {
                    ADDMSG(&captain->faction->msgs,
                        msg_message("detectforbidden", "unit region", u, r));
                }

                return SA_NO_INSECT;
            }
        }
    }

    if (bt_harbour && buildingtype_exists(r, bt_harbour, true)) {
        unit* harbourmaster = NULL;
        harbourmaster = owner_buildingtyp(r, bt_harbour);
        if (!harbourmaster || !sh->_owner) {
            return SA_HARBOUR;
        }
        else if ((sh->_owner->faction == harbourmaster->faction) || (ucontact(harbourmaster, sh->_owner)) || (alliedunit(harbourmaster, sh->_owner->faction, HELP_GUARD))) {
            return SA_HARBOUR;
        }
    }
    if (fval(r->terrain, SEA_REGION)) {
        return SA_COAST;
    }
    if (sh->type->coasts) {
        for (c = 0; sh->type->coasts[c] != NULL; ++c) {
            if (sh->type->coasts[c] == r->terrain) {
                return SA_COAST;
            }
        }
    }
    return SA_NO_COAST;
}

static bool flying_ship(const ship * sh)
{
    if (sh->type->flags & SFL_FLY)
        return true;
    if (sh->flags & SF_FLYING)
        return true;
    return false;
}

static void set_coast(ship * sh, region * r, region * rnext)
{
    if (sh->type->flags & SFL_NOCOAST) {
        sh->coast = NODIRECTION;
    }
    else if (!fval(rnext->terrain, SEA_REGION) && !flying_ship(sh)) {
        sh->coast = reldirection(rnext, r);
        assert(fval(r->terrain, SEA_REGION));
    }
    else {
        sh->coast = NODIRECTION;
    }
}

static float damage_drift(void)
{
    static float value = -1.0F;
    if (value < 0) {
        value = get_param_flt(global.parameters, "rules.ship.damage_drift", 0.02F);
    }
    return value;
}

static void drifting_ships(region * r)
{
    direction_t d;
    bool drift = get_param_int(global.parameters, "rules.ship.drifting", 1) != 0;

    if (fval(r->terrain, SEA_REGION)) {
        ship **shp = &r->ships;
        while (*shp) {
            ship *sh = *shp;
            region *rnext = NULL;
            region_list *route = NULL;
            unit *firstu = NULL, *captain;
            int d_offset;
            direction_t dir = 0;

            if (sh->type->fishing > 0) {
                sh->flags |= SF_FISHING;
            }

            /* Schiff schon abgetrieben oder durch Zauber geschützt? */
            if (!drift || fval(sh, SF_DRIFTED) || is_cursed(sh->attribs, C_SHIP_NODRIFT, 0)) {
                shp = &sh->next;
                continue;
            }

            /* Kapitän bestimmen */
            for (captain = r->units; captain; captain = captain->next) {
                if (captain->ship != sh)
                    continue;
                if (firstu == NULL)
                    firstu = captain;
                if (eff_skill(captain, SK_SAILING, r) >= sh->type->cptskill) {
                    break;
                }
            }
            /* Kapitän da? Beschädigt? Genügend Matrosen?
             * Genügend leicht? Dann ist alles OK. */

            assert(sh->type->construction->improvement == NULL);      /* sonst ist construction::size nicht ship_type::maxsize */
            if (captain && sh->size == sh->type->construction->maxsize
                && enoughsailors(sh, crew_skill(sh)) && cansail(r, sh)) {
                shp = &sh->next;
                continue;
            }

            /* Auswahl einer Richtung: Zuerst auf Land, dann
             * zufällig. Falls unmögliches Resultat: vergiß es. */
            d_offset = rng_int() % MAXDIRECTIONS;
            for (d = 0; d != MAXDIRECTIONS; ++d) {
                region *rn;
                dir = (direction_t)((d + d_offset) % MAXDIRECTIONS);
                rn = rconnect(r, dir);
                if (rn != NULL && fval(rn->terrain, SAIL_INTO) && check_ship_allowed(sh, rn) > 0) {
                    rnext = rn;
                    if (!fval(rnext->terrain, SEA_REGION))
                        break;
                }
            }

            if (rnext == NULL) {
                shp = &sh->next;
                continue;
            }

            /* Das Schiff und alle Einheiten darin werden nun von r
             * nach rnext verschoben. Danach eine Meldung. */
            add_regionlist(&route, rnext);

            set_coast(sh, r, rnext);
            sh = move_ship(sh, r, rnext, route);
            free_regionlist(route);

            if (firstu != NULL) {
                unit *u, *lastu = NULL;
                message *msg = msg_message("ship_drift", "ship dir", sh, dir);
                for (u = firstu; u; u = u->next) {
                    if (u->ship == sh && !fval(u->faction, FFL_MARK)) {
                        fset(u->faction, FFL_MARK);
                        add_message(&u->faction->msgs, msg);
                        lastu = u->next;
                    }
                }
                for (u = firstu; u != lastu; u = u->next) {
                    freset(u->faction, FFL_MARK);
                }
                msg_release(msg);
            }

            if (sh != NULL) {
                fset(sh, SF_DRIFTED);

                damage_ship(sh, damage_drift());
                if (sh->damage >= sh->size * DAMAGE_SCALE) {
                    remove_ship(&sh->region->ships, sh);
                }
            }

            if (*shp == sh)
                shp = &sh->next;
        }
    }
}

static bool present(region * r, unit * u)
{
    return (bool)(u && u->region == r);
}

static void caught_target(region * r, unit * u)
{
    attrib *a = a_find(u->attribs, &at_follow);

    /* Verfolgungen melden */
    /* Misserfolgsmeldung, oder bei erfolgreichem Verfolgen unter
     * Umstaenden eine Warnung. */

    if (a) {
        unit *target = (unit *)a->data.v;

        if (!present(r, target)) {
            ADDMSG(&u->faction->msgs, msg_message("followfail", "unit follower",
                target, u));
        }
        else if (!alliedunit(target, u->faction, HELP_ALL)
            && cansee(target->faction, r, u, 0)) {
            ADDMSG(&target->faction->msgs, msg_message("followdetect",
                "unit follower", target, u));
        }
    }
}

static unit *bewegung_blockiert_von(unit * reisender, region * r)
{
    unit *u;
    double prob = 0.0;
    unit *guard = NULL;
    int guard_count = 0;
    int stealth = eff_stealth(reisender, r);
    static int gamecookie = -1;
    static double base_prob = -999;
    static double skill_prob = -999;
    static double amulet_prob = -999;
    static double guard_number_prob = -999;
    static double castle_prob = -999;
    static double region_type_prob = -999;
    const struct resource_type *ramulet = get_resourcetype(R_AMULET_OF_TRUE_SEEING);

    if (gamecookie < 0 || gamecookie != global.cookie) {
        base_prob = get_param_flt(global.parameters, "rules.guard.base_stop_prob", .3f);
        skill_prob = get_param_flt(global.parameters, "rules.guard.skill_stop_prob", .1f);
        amulet_prob = get_param_flt(global.parameters, "rules.guard.amulet_stop_prob", .1f);
        guard_number_prob = get_param_flt(global.parameters, "rules.guard.guard_number_stop_prob", .001f);
        castle_prob = get_param_flt(global.parameters, "rules.guard.castle_stop_prob", .1f);
        region_type_prob = get_param_flt(global.parameters, "rules.guard.region_type_stop_prob", .1f);
        gamecookie = global.cookie;
    }

    if (fval(u_race(reisender), RCF_ILLUSIONARY))
        return NULL;
    for (u = r->units; u; u = u->next) {
        if (is_guard(u, GUARD_TRAVELTHRU)) {
            int sk = eff_skill(u, SK_PERCEPTION, r);
            if (invisible(reisender, u) >= reisender->number)
                continue;
            if (!(u_race(u)->flags & RCF_FLY) && u_race(reisender)->flags & RCF_FLY)
                continue;
            if ((u->faction == reisender->faction) || (ucontact(u, reisender)) || (alliedunit(u, reisender->faction, HELP_GUARD)))
                guard_count = guard_count - u->number;
            else if (sk >= stealth) {
                guard_count += u->number;
                double prob_u = (sk - stealth) * skill_prob;
                /* amulet counts at most once */
                prob_u += _min(1, _min(u->number, i_get(u->items, ramulet->itype))) * amulet_prob;
                if (u->building && (u->building->type == bt_find("castle")) && u == building_owner(u->building))
                    prob_u += castle_prob*buildingeffsize(u->building, 0);
                if (prob_u >= prob) {
                    prob = prob_u;
                    guard = u;
                }
            }
        }
    }
    if (guard) {
        prob += base_prob;          /* 30% base chance */
        prob += guard_count*guard_number_prob;
        if (r->terrain == newterrain(T_GLACIER))
            prob += region_type_prob * 2;
        if (r->terrain == newterrain(T_SWAMP))
            prob += region_type_prob * 2;
        if (r->terrain == newterrain(T_MOUNTAIN))
            prob += region_type_prob;
        if (r->terrain == newterrain(T_VOLCANO))
            prob += region_type_prob;
        if (r->terrain == newterrain(T_VOLCANO_SMOKING))
            prob += region_type_prob;

        if (prob > 0 && chance(prob)) {
            return guard;
        }
    }
    return NULL;
}

static bool is_guardian_u(const unit * guard, unit * u, unsigned int mask)
{
    if (guard->faction == u->faction)
        return false;
    if (is_guard(guard, mask) == 0)
        return false;
    if (alliedunit(guard, u->faction, HELP_GUARD))
        return false;
    if (ucontact(guard, u))
        return false;
    if (!cansee(guard->faction, u->region, u, 0))
        return false;
    if (!(u_race(guard)->flags & RCF_FLY) && u_race(u)->flags & RCF_FLY)
        return false;

    return true;
}

static bool is_guardian_r(const unit * guard)
{
    if (guard->number == 0)
        return false;
    if (besieged(guard))
        return false;

    /* if region_owners exist then they may be guardians: */
    if (guard->building && rule_region_owners() && guard == building_owner(guard->building)) {
        faction *owner = region_get_owner(guard->region);
        if (owner == guard->faction) {
            building *bowner = largestbuilding(guard->region, &cmp_taxes, false);
            if (bowner == guard->building) {
                return true;
            }
        }
    }

    if ((guard->flags & UFL_GUARD) == 0)
        return false;
    if (!armedmen(guard, true) && !fval(u_race(guard), RCF_UNARMEDGUARD))
        return false;
    return true;
}

bool is_guard(const struct unit * u, int mask)
{
    return is_guardian_r(u) && (getguard(u) & mask) != 0;
}

#define MAXGUARDCACHE 16
/** returns the guard which prevents 'u' from doing 'mask' actions in 'r'.
*/
unit *is_guarded(region * r, unit * u, unsigned int mask)
{
    unit *u2 = NULL;
    int i, noguards = 1;
    static unit *guardcache[MAXGUARDCACHE], *lastguard;   /* STATIC_XCALL: used across calls */
    static int gamecookie = -1;

    if (!fval(r, RF_GUARDED)) {
        return NULL;
    }

    if (gamecookie != global.cookie) {
        if (gamecookie >= 0) {
            /* clear the previous turn's cache */
            memset(guardcache, 0, sizeof(guardcache));
            lastguard = NULL;
        }
        gamecookie = global.cookie;
    }

    if (lastguard && lastguard->region == r) {
        if (is_guardian_u(lastguard, u, mask)) {
            return lastguard;
        }
    }

    for (i = 0; i != MAXGUARDCACHE; ++i) {
        unit *guard = guardcache[i];
        if (guard && guard != lastguard && guard->region == r) {
            noguards = 0;
            if (is_guardian_u(guard, u, mask)) {
                lastguard = guard;
                return guard;
            }
            if (u2 == guard) {
                /* same guard twice signals we've tested everyone */
                return NULL;
            }
            u2 = guard;
        }
        else {
            /* exhausted all the guards in the cache, but maybe we'll find one later? */
            break;
        }
    }

    /* at this point, u2 is the last unit we tested to
     * be a guard (and failed), or NULL
     * i is the position of the first free slot in the cache */

    for (u2 = (u2 ? u2->next : r->units); u2; u2 = u2->next) {
        if (is_guardian_r(u2)) {
            noguards = 0;
            /* u2 is a guard, so worth remembering */
            if (i < MAXGUARDCACHE)
                guardcache[i++] = u2;
            if (is_guardian_u(u2, u, mask)) {
                /* u2 is our guard. stop processing (we might have to go further next time) */
                lastguard = u2;
                return u2;
            }
        }
    }
    /* there are no more guards. we signal this by duplicating the last one.
     * i is still the position of the first free slot in the cache */
    if (i > 0 && i < MAXGUARDCACHE) {
        guardcache[i] = guardcache[i - 1];
    }

    if (noguards) {
        /* you are mistaken, sir. there are no guards in these lands */
        freset(r, RF_GUARDED);
    }
    return NULL;
}

bool move_blocked(const unit * u, const region * r, const region * r2)
{
    connection *b;
    curse *c;
    static const curse_type *fogtrap_ct = NULL;

    if (r2 == NULL)
        return true;
    b = get_borders(r, r2);
    while (b) {
        if (b->type->block && b->type->block(b, u, r))
            return true;
        b = b->next;
    }

    if (fogtrap_ct == NULL)
        fogtrap_ct = ct_find("fogtrap");
    c = get_curse(r->attribs, fogtrap_ct);
    if (curse_active(c))
        return true;
    return false;
}

int movewhere(const unit * u, const char *token, region * r, region ** resultp)
{
    region *r2;
    direction_t d;

    if (!token || *token == '\0') {
        *resultp = NULL;
        return E_MOVE_OK;
    }

    d = get_direction(token, u->faction->locale);
    switch (d) {
    case D_PAUSE:
        *resultp = r;
        break;

    case NODIRECTION:
        token = (const char *)get_translation(u->faction->locale, token, UT_SPECDIR);
        if (!token) {
            return E_MOVE_NOREGION;
        }
        r2 = find_special_direction(r, token);
        if (r2 == NULL) {
            return E_MOVE_NOREGION;
        }
        *resultp = r2;
        break;

    default:
        r2 = rconnect(r, d);
        if (r2 == NULL || move_blocked(u, r, r2)) {
            return E_MOVE_BLOCKED;
        }
        *resultp = r2;
    }
    return E_MOVE_OK;
}

static const char *shortdirections[MAXDIRECTIONS] = {
    "dir_nw",
    "dir_ne",
    "dir_east",
    "dir_se",
    "dir_sw",
    "dir_west"
};

static void cycle_route(order * ord, unit * u, int gereist)
{
    int bytes, cm = 0;
    char tail[1024], *bufp = tail;
    char neworder[2048];
    char token[128];
    direction_t d = NODIRECTION;
    bool paused = false;
    bool pause;
    order *norder;
    size_t size = sizeof(tail) - 1;

    if (getkeyword(ord) != K_ROUTE)
        return;
    tail[0] = '\0';

    init_order(ord);

    neworder[0] = 0;
    for (cm = 0;; ++cm) {
        const char *s;
        const struct locale *lang = u->faction->locale;
        pause = false;
        s = gettoken(token, sizeof(token));
        if (s && *s) {
            d = get_direction(s, lang);
            if (d == D_PAUSE) {
                pause = true;
            }
            else if (d == NODIRECTION) {
                break;
            }
        }
        else {
            break;
        }
        if (cm < gereist) {
            /* hier sollte keine PAUSE auftreten */
            assert(!pause);
            if (!pause) {
                const char *loc = LOC(lang, shortdirections[d]);
                if (bufp != tail) {
                    bytes = (int)strlcpy(bufp, " ", size);
                    if (wrptr(&bufp, &size, bytes) != 0)
                        WARN_STATIC_BUFFER();
                }
                bytes = (int)strlcpy(bufp, loc, size);
                if (wrptr(&bufp, &size, bytes) != 0)
                    WARN_STATIC_BUFFER();
            }
        }
        else if (strlen(neworder) > sizeof(neworder) / 2)
            break;
        else if (cm == gereist && !paused && pause) {
            const char *loc = LOC(lang, parameters[P_PAUSE]);
            bytes = (int)strlcpy(bufp, " ", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, loc, size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
            paused = true;
        }
        else if (pause) {
            /* da PAUSE nicht in ein shortdirections[d] umgesetzt wird (ist
             * hier keine normale direction), muss jede PAUSE einzeln
             * herausgefiltert und explizit gesetzt werden */
            if (neworder[0])
                strcat(neworder, " ");
            strcat(neworder, LOC(lang, parameters[P_PAUSE]));
        }
        else {
            if (neworder[0])
                strcat(neworder, " ");
            strcat(neworder, LOC(lang, shortdirections[d]));
        }
    }

    if (neworder[0]) {
        norder = create_order(K_ROUTE, u->faction->locale, "%s %s", neworder, tail);
    }
    else {
        norder = create_order(K_ROUTE, u->faction->locale, "%s", tail);
    }
    replace_order(&u->orders, ord, norder);
    free_order(norder);
}

static bool transport(unit * ut, unit * u)
{
    order *ord;

    if (LongHunger(u) || fval(ut->region->terrain, SEA_REGION)) {
        return false;
    }

    for (ord = ut->orders; ord; ord = ord->next) {
        if (getkeyword(ord) == K_TRANSPORT) {
            unit *u2;
            init_order(ord);
            getunit(ut->region, ut->faction, &u2);
            if (u2 == u) {
                return true;
            }
        }
    }
    return false;
}

static bool can_move(const unit * u)
{
    if (u_race(u)->flags & RCF_CANNOTMOVE)
        return false;
    if (get_movement(&u->attribs, MV_CANNOTMOVE))
        return false;
    return true;
}

static void init_transportation(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        unit *u;

        /* This is just a simple check for non-corresponding K_TRANSPORT/
         * K_DRIVE. This is time consuming for an error check, but there
         * doesn't seem to be an easy way to speed this up. */
        for (u = r->units; u; u = u->next) {
            if (getkeyword(u->thisorder) == K_DRIVE && can_move(u)
                && !fval(u, UFL_NOTMOVING) && !LongHunger(u)) {
                unit *ut = 0;

                init_order(u->thisorder);
                if (getunit(r, u->faction, &ut) != GET_UNIT) {
                    ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
                        "feedback_unit_not_found", ""));
                    continue;
                }
                if (!transport(ut, u)) {
                    if (cansee(u->faction, r, ut, 0)) {
                        cmistake(u, u->thisorder, 286, MSG_MOVE);
                    }
                    else {
                        ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
                            "feedback_unit_not_found", ""));
                    }
                }
            }
        }

        /* This calculates the weights of all transported units and
         * adds them to an internal counter which is used by travel () to
         * calculate effective weight and movement. */

        if (!fval(r->terrain, SEA_REGION)) {
            for (u = r->units; u; u = u->next) {
                order *ord;
                int w = 0;

                for (ord = u->orders; ord; ord = ord->next) {
                    if (getkeyword(ord) == K_TRANSPORT) {
                        init_order(ord);
                        for (;;) {
                            unit *ut = 0;

                            if (getunit(r, u->faction, &ut) != GET_UNIT) {
                                break;
                            }
                            if (getkeyword(ut->thisorder) == K_DRIVE &&
                                can_move(ut) && !fval(ut, UFL_NOTMOVING) &&
                                !LongHunger(ut)) {
                                unit *u2;
                                init_order(ut->thisorder);
                                getunit(r, ut->faction, &u2);
                                if (u2 == u) {
                                    w += weight(ut);
                                }
                            }
                        }
                    }
                }
                if (w > 0)
                    a_add(&u->attribs, a_new(&at_driveweight))->data.i = w;
            }
        }
    }
}

static bool roadto(const region * r, direction_t dir)
{
    /* wenn es hier genug strassen gibt, und verbunden ist, und es dort
     * genug strassen gibt, dann existiert eine strasse in diese richtung */
    region *r2;
    static const curse_type *roads_ct = NULL;

    if (dir >= MAXDIRECTIONS || dir < 0)
        return false;
    r2 = rconnect(r, dir);
    if (r == NULL || r2 == NULL)
        return false;

    if (roads_ct == NULL)
        roads_ct = ct_find("magicstreet");
    if (roads_ct != NULL) {
        if (get_curse(r->attribs, roads_ct) != NULL)
            return true;
        if (get_curse(r2->attribs, roads_ct) != NULL)
            return true;
    }

    if (r->terrain->max_road <= 0)
        return false;
    if (r2->terrain->max_road <= 0)
        return false;
    if (rroad(r, dir) < r->terrain->max_road)
        return false;
    if (rroad(r2, dir_invert(dir)) < r2->terrain->max_road)
        return false;
    return true;
}

static const region_list *cap_route(region * r, const region_list * route,
    const region_list * route_end, int speed)
{
    region *current = r;
    int moves = speed;
    const region_list *iroute = route;
    while (iroute != route_end) {
        region *next = iroute->data;
        direction_t reldir = reldirection(current, next);

        /* adjust the range of the unit */
        if (roadto(current, reldir))
            moves -= BP_ROAD;
        else
            moves -= BP_NORMAL;
        if (moves < 0)
            break;
        iroute = iroute->next;
        current = next;
    }
    return iroute;
}

static region *next_region(unit * u, region * current, region * next)
{
    connection *b;

    b = get_borders(current, next);
    while (b != NULL) {
        if (b->type->move) {
            region *rto = b->type->move(b, u, current, next, true);
            if (rto != next) {
                /* the target region was changed (wisps, for example). check the
                 * new target region for borders */
                next = rto;
                b = get_borders(current, next);
                continue;
            }
        }
        b = b->next;
    }
    return next;
}

static const region_list *reroute(unit * u, const region_list * route,
    const region_list * route_end)
{
    region *current = u->region;
    while (route != route_end) {
        region *next = next_region(u, current, route->data);
        if (next != route->data)
            break;
        route = route->next;
    }
    return route;
}

static message *movement_error(unit * u, const char *token, order * ord,
    int error_code)
{
    direction_t d;
    switch (error_code) {
    case E_MOVE_BLOCKED:
        d = get_direction(token, u->faction->locale);
        return msg_message("moveblocked", "unit direction", u, d);
    case E_MOVE_NOREGION:
        return msg_feedback(u, ord, "unknowndirection", "dirname", token);
    }
    return NULL;
}

static void make_route(unit * u, order * ord, region_list ** routep)
{
    region_list **iroute = routep;
    region *current = u->region;
    region *next = NULL;
    char token[128];
    const char *s = gettoken(token, sizeof(token));
    int error = movewhere(u, s, current, &next);

    if (error != E_MOVE_OK) {
        message *msg = movement_error(u, s, ord, error);
        if (msg != NULL) {
            add_message(&u->faction->msgs, msg);
            msg_release(msg);
        }
        next = NULL;
    }

    while (next != NULL) {
        if (current == next) {
            /* PAUSE */
            break;
        }
        next = next_region(u, current, next);

        add_regionlist(iroute, next);
        iroute = &(*iroute)->next;

        current = next;
        s = gettoken(token, sizeof(token));
        error = movewhere(u, s, current, &next);
        if (error) {
            message *msg = movement_error(u, s, ord, error);
            if (msg != NULL) {
                add_message(&u->faction->msgs, msg);
                msg_release(msg);
            }
            next = NULL;
        }
    }
}

/** calculate the speed of a unit
 *
 * zu Fuß reist man 1 Region, zu Pferd 2 Regionen. Mit Straßen reist
 * man zu Fuß 2, mit Pferden 3 weit.
 *
 * Berechnet wird das mit BPs. Zu Fuß hat man 4 BPs, zu Pferd 6.
 * Normalerweise verliert man 3 BP pro Region, bei Straßen nur 2 BP.
 * Außerdem: Wenn Einheit transportiert, nur halbe BP
 */
static int movement_speed(unit * u)
{
    int mp;
    static const curse_type *speed_ct;
    static bool init = false;
    double dk = u_race(u)->speed;

    assert(u->number);
    /* dragons have a fixed speed, and no other effects work on them: */
    switch (old_race(u_race(u))) {
    case RC_DRAGON:
    case RC_WYRM:
    case RC_FIREDRAGON:
    case RC_BIRTHDAYDRAGON:
    case RC_SONGDRAGON:
        return BP_DRAGON;
    default:
        break;
    }

    if (!init) {
        init = true;
        speed_ct = ct_find("speed");
    }
    if (speed_ct) {
        curse *c = get_curse(u->attribs, speed_ct);
        if (c != NULL) {
            int men = get_cursedmen(u, c);
            dk *= 1.0 + (double)men / (double)u->number;
        }
    }

    switch (canride(u)) {

    case 1:                      /* Pferd */
        mp = BP_RIDING;
        break;

    case 2:                      /* Einhorn */
        mp = BP_UNICORN;
        break;

    default:
        mp = BP_WALKING;

        /* Siebenmeilentee */
        if (get_effect(u, oldpotiontype[P_FAST]) >= u->number) {
            mp *= 2;
            change_effect(u, oldpotiontype[P_FAST], -u->number);
        }

        /* unicorn in inventory */
        if (u->number <= i_get(u->items, it_find("fairyboot"))) {
            mp *= 2;
        }

        /* Im Astralraum sind Tyb und Ill-Magier doppelt so schnell.
         * Nicht kumulativ mit anderen Beschleunigungen! */
        if (mp * dk <= BP_WALKING * u_race(u)->speed && is_astral(u->region)
            && is_mage(u)) {
            sc_mage *mage = get_mage(u);
            if (mage->magietyp == M_TYBIED || mage->magietyp == M_ILLAUN) {
                mp *= 2;
            }
        }
        break;
    }
    return (int)(dk * mp);
}

enum {
    TRAVEL_NORMAL,
    TRAVEL_FOLLOWING,
    TRAVEL_TRANSPORTED,
    TRAVEL_RUNNING
};

static arg_regions *var_copy_regions(const region_list * begin, int size)
{
    const region_list *rsrc;

    if (size > 0) {
        int i = 0;
        arg_regions *dst =
            (arg_regions *)malloc(sizeof(arg_regions) + sizeof(region *) * size);
        dst->nregions = size;
        dst->regions = (region **)(dst + 1);
        for (rsrc = begin; i != size; rsrc = rsrc->next) {
            dst->regions[i++] = rsrc->data;
        }
        return dst;
    }
    return NULL;
}

static const region_list *travel_route(unit * u,
    const region_list * route_begin, const region_list * route_end, order * ord,
    int mode)
{
    region *r = u->region;
    region *current = u->region;
    const region_list *iroute = route_begin;
    int steps = 0;
    bool landing = false;      /* aquarians have landed */

    while (iroute && iroute != route_end) {
        region *next = iroute->data;
        direction_t reldir = reldirection(current, next);
        connection *b = get_borders(current, next);

        /* check if we are caught by guarding units */
        if (iroute != route_begin && mode != TRAVEL_RUNNING
            && mode != TRAVEL_TRANSPORTED) {
            unit *wache = bewegung_blockiert_von(u, current);
            if (wache != NULL) {
                ADDMSG(&u->faction->msgs, msg_message("moveblockedbyguard",
                    "unit region guard", u, current, wache));
                break;
            }
        }

        /* check movement from/to oceans.
         * aquarian special, flying units, horses, the works */
        if ((u_race(u)->flags & RCF_FLY) == 0) {
            if (!fval(next->terrain, SEA_REGION)) {
                /* next region is land */
                if (fval(current->terrain, SEA_REGION)) {
                    int moving = u_race(u)->flags & (RCF_SWIM | RCF_WALK | RCF_COASTAL);
                    /* Die Einheit kann nicht fliegen, ist im Ozean, und will an Land */
                    if (moving != (RCF_SWIM | RCF_WALK) && (moving & RCF_COASTAL) == 0) {
                        /* can't swim+walk and isn't allowed to enter coast from sea */
                        if (ord != NULL)
                            cmistake(u, ord, 44, MSG_MOVE);
                        break;
                    }
                    landing = true;
                }
                else if ((u_race(u)->flags & RCF_WALK) == 0) {
                    /* Spezialeinheiten, die nicht laufen können. */
                    ADDMSG(&u->faction->msgs, msg_message("detectocean",
                        "unit region", u, next));
                    break;
                }
                else if (landing) {
                    /* wir sind diese woche angelandet */
                    ADDMSG(&u->faction->msgs, msg_message("detectocean",
                        "unit region", u, next));
                    break;
                }
            }
            else {
                /* Ozeanfelder können nur von Einheiten mit Schwimmen und ohne
                 * Pferde betreten werden. */
                if (!(canswim(u) || canfly(u))) {
                    ADDMSG(&u->faction->msgs, msg_message("detectocean",
                        "unit region", u, next));
                    break;
                }
            }

            if (fval(current->terrain, SEA_REGION) || fval(next->terrain, SEA_REGION)) {
                /* trying to enter or exit ocean with horses, are we? */
                if (has_horses(u)) {
                    /* tries to do it with horses */
                    if (ord != NULL)
                        cmistake(u, ord, 67, MSG_MOVE);
                    break;
                }
            }

        }

        /* movement blocked by a wall */
        if (reldir >= 0 && move_blocked(u, current, next)) {
            ADDMSG(&u->faction->msgs, msg_message("leavefail",
                "unit region", u, next));
            break;
        }

        /* region ownership only: region owned by enemies */
        if (!entrance_allowed(u, next)) {
            ADDMSG(&u->faction->msgs, msg_message("regionowned",
                "unit region target", u, current, next));
            break;
        }

        /* illusionary units disappear in antimagic zones */
        if (fval(u_race(u), RCF_ILLUSIONARY)) {
            curse *c = get_curse(next->attribs, ct_find("antimagiczone"));
            if (curse_active(c)) {
                curse_changevigour(&next->attribs, c, (float)-u->number);
                ADDMSG(&u->faction->msgs, msg_message("illusionantimagic", "unit", u));
                set_number(u, 0);
                break;
            }
        }

        /* terrain is marked as forbidden (curse, etc) */
        if (fval(next, RF_BLOCKED) || fval(next->terrain, FORBIDDEN_REGION)) {
            ADDMSG(&u->faction->msgs, msg_message("detectforbidden",
                "unit region", u, next));
            break;
        }

        /* unit is an insect and cannot move into a glacier */
        if (u_race(u) == get_race(RC_INSECT)) {
            if (r_insectstalled(next) && is_freezing(u)) {
                ADDMSG(&u->faction->msgs, msg_message("detectforbidden",
                    "unit region", u, next));
                break;
            }
        }

        /* effect of borders */
        while (b != NULL) {
            if (b->type->move) {
                b->type->move(b, u, current, next, false);
            }
            b = b->next;
        }

        current = next;
        iroute = iroute->next;
        ++steps;
        if (u->number == 0)
            break;
    }

    if (iroute != route_begin) {
        /* the unit has moved at least one region */
        int walkmode;

        setguard(u, GUARD_NONE);
        cycle_route(ord, u, steps);

        if (mode == TRAVEL_RUNNING) {
            walkmode = 0;
        }
        if (canride(u)) {
            walkmode = 1;
            produceexp(u, SK_RIDING, u->number);
        }
        else {
            walkmode = 2;
        }

        /* Berichte über Durchreiseregionen */

        if (mode != TRAVEL_TRANSPORTED) {
            arg_regions *ar = var_copy_regions(route_begin, steps - 1);
            ADDMSG(&u->faction->msgs, msg_message("travel",
                "unit mode start end regions", u, walkmode, r, current, ar));
        }

        mark_travelthru(u, r, route_begin, iroute);
        move_unit(u, current, NULL);

        /* make orders for the followers */
    }
    fset(u, UFL_LONGACTION | UFL_NOTMOVING);
    setguard(u, GUARD_NONE);
    assert(u->region == current);
    return iroute;
}

static bool ship_ready(const region * r, unit * u)
{
    if (!u->ship || u != ship_owner(u->ship)) {
        cmistake(u, u->thisorder, 146, MSG_MOVE);
        return false;
    }
    if (eff_skill(u, SK_SAILING, r) < u->ship->type->cptskill) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
            "error_captain_skill_low", "value ship", u->ship->type->cptskill,
            u->ship));
        return false;
    }
    assert(u->ship->type->construction->improvement == NULL);     /* sonst ist construction::size nicht ship_type::maxsize */
    if (u->ship->size != u->ship->type->construction->maxsize) {
        cmistake(u, u->thisorder, 15, MSG_MOVE);
        return false;
    }
    if (!enoughsailors(u->ship, crew_skill(u->ship))) {
        cmistake(u, u->thisorder, 1, MSG_MOVE);
        /*		mistake(u, u->thisorder,
                                        "Auf dem Schiff befinden sich zuwenig erfahrene Seeleute.", MSG_MOVE); */
        return false;
    }
    if (!cansail(r, u->ship)) {
        cmistake(u, u->thisorder, 18, MSG_MOVE);
        return false;
    }
    return true;
}

unit *owner_buildingtyp(const region * r, const building_type * bt)
{
    building *b;
    unit *owner;

    for (b = rbuildings(r); b; b = b->next) {
        owner = building_owner(b);
        if (b->type == bt && owner != NULL) {
            if (b->size >= bt->maxsize) {
                return owner;
            }
        }
    }

    return NULL;
}

bool
buildingtype_exists(const region * r, const building_type * bt, bool working)
{
    building *b;

    for (b = rbuildings(r); b; b = b->next) {
        if (b->type == bt && b->size >= bt->maxsize && (!working || fval(b, BLD_WORKING)))
            return true;
    }

    return false;
}

/* Prüft, ob Ablegen von einer Küste in eine der erlaubten Richtungen erfolgt. */

static bool check_takeoff(ship * sh, region * from, region * to)
{
    if (!fval(from->terrain, SEA_REGION) && sh->coast != NODIRECTION) {
        direction_t coast = sh->coast;
        direction_t dir = reldirection(from, to);
        direction_t coastr = (direction_t)((coast + 1) % MAXDIRECTIONS);
        direction_t coastl =
            (direction_t)((coast + MAXDIRECTIONS - 1) % MAXDIRECTIONS);

        if (dir != coast && dir != coastl && dir != coastr
            && !buildingtype_exists(from, bt_find("harbour"), true)) {
            return false;
        }
    }

    return true;
}

static void
sail(unit * u, order * ord, bool move_on_land, region_list ** routep)
{
    region *starting_point = u->region;
    region *current_point, *last_point;
    int k, step = 0;
    region_list **iroute = routep;
    ship *sh = u->ship;
    faction *f = u->faction;
    region *next_point = NULL;
    int error;
    const char *token = getstrtoken();

    if (routep)
        *routep = NULL;

    error = movewhere(u, token, starting_point, &next_point);
    if (error) {
        message *msg = movement_error(u, token, ord, error);
        if (msg != NULL) {
            add_message(&u->faction->msgs, msg);
            msg_release(msg);
        }
        return;
    }

    if (!ship_ready(starting_point, u))
        return;

    /* Wir suchen so lange nach neuen Richtungen, wie es geht. Diese werden
     * dann nacheinander ausgeführt. */

    k = shipspeed(sh, u);

    last_point = starting_point;
    current_point = starting_point;

    /* die nächste Region, in die man segelt, wird durch movewhere () aus der
     * letzten Region bestimmt.
     *
     * Anfangen tun wir bei starting_point. next_point ist beim ersten
     * Durchlauf schon gesetzt (Parameter!). current_point ist die letzte gültige,
     * befahrene Region. */

    while (next_point && current_point != next_point && step < k) {
        const char *token;
        int error;
        const terrain_type *tthis = current_point->terrain;
        /* these values need to be updated if next_point changes (due to storms): */
        const terrain_type *tnext = next_point->terrain;

        assert(sh == u->ship || !"ship has sunk, but we didn't notice it");

        if (fval(next_point->terrain, FORBIDDEN_REGION) || fval(next_point, RF_BLOCKED)) {
            ADDMSG(&f->msgs, msg_message("sailforbidden",
                "ship region", sh, next_point));
            break;
        }

        if (!flying_ship(sh)) {
            int stormchance;
            static int stormyness;
            static int gamecookie = -1;
            int reason;

            if (gamecookie != global.cookie) {
                bool storms_enabled = get_param_int(global.parameters, "rules.ship.storms", 1) != 0;
                if (storms_enabled) {
                    gamedate date;
                    get_gamedate(turn, &date);
                    stormyness = storms ? storms[date.month] * 5 : 0;
                }
                gamecookie = global.cookie;
            }

            /* storms should be the first thing we do. */
            stormchance = stormyness / shipspeed(sh, u);
            if (check_leuchtturm(next_point, NULL)) {
                int param = get_param_int(global.parameters, "rules.lighthous.stormchancedevisor", 0);
                if (param > 0) {
                    stormchance /= param;
                }
                else {
                    stormchance = 0;
                }
            }
            if (rng_int() % 10000 < stormchance * sh->type->storm
                && fval(current_point->terrain, SEA_REGION)) {
                if (!is_cursed(sh->attribs, C_SHIP_NODRIFT, 0)) {
                    region *rnext = NULL;
                    bool storm = true;
                    int d_offset = rng_int() % MAXDIRECTIONS;
                    direction_t d;
                    /* Sturm nur, wenn nächste Region Hochsee ist. */
                    for (d = 0; d != MAXDIRECTIONS; ++d) {
                        direction_t dnext = (direction_t)((d + d_offset) % MAXDIRECTIONS);
                        region *rn = rconnect(current_point, dnext);

                        if (rn != NULL) {
                            if (fval(rn->terrain, FORBIDDEN_REGION))
                                continue;
                            if (!fval(rn->terrain, SEA_REGION)) {
                                storm = false;
                                break;
                            }
                            if (rn != next_point)
                                rnext = rn;
                        }
                    }
                    if (storm && rnext != NULL) {
                        ADDMSG(&f->msgs, msg_message("storm", "ship region sink",
                            sh, current_point, sh->damage >= sh->size * DAMAGE_SCALE));

                        /* damage the ship. we handle destruction in the end */
                        damage_ship(sh, damage_drift());
                        if (sh->damage >= sh->size * DAMAGE_SCALE)
                            break;

                        next_point = rnext;
                        /* these values need to be updated if next_point changes (due to storms): */
                        tnext = next_point->terrain;
                    }
                }
            }

            if (!fval(tthis, SEA_REGION)) {
                if (!fval(tnext, SEA_REGION)) {
                    if (!move_on_land) {
                        /* check that you're not traveling from one land region to another. */
                        ADDMSG(&u->faction->msgs, msg_message("shipnoshore",
                            "ship region", sh, next_point));
                        break;
                    }
                }
                else {
                    if (!check_takeoff(sh, current_point, next_point)) {
                        /* Schiff kann nicht ablegen */
                        cmistake(u, ord, 182, MSG_MOVE);
                        break;
                    }
                }
            }
            else if (fval(tnext, SEA_REGION)) {
                /* target region is an ocean, and we're not leaving a shore */
                if (!(sh->type->flags & SFL_OPENSEA)) {
                    /* ship can only stay close to shore */
                    direction_t d;

                    for (d = 0; d != MAXDIRECTIONS; ++d) {
                        region *rc = rconnect(next_point, d);
                        if (rc == NULL || !fval(rc->terrain, SEA_REGION))
                            break;
                    }
                    if (d == MAXDIRECTIONS) {
                        /* Schiff kann nicht aufs offene Meer */
                        cmistake(u, ord, 249, MSG_MOVE);
                        break;
                    }
                }
            }

            reason = check_ship_allowed(sh, next_point);
            if (reason < 0) {
                /* for some reason or another, we aren't allowed in there.. */
                if (check_leuchtturm(current_point, NULL) || reason == SA_NO_INSECT) {
                    ADDMSG(&f->msgs, msg_message("sailnolandingstorm", "ship region", sh, next_point));
                }
                else {
                    float dmg =
                        get_param_flt(global.parameters, "rules.ship.damage.nolanding",
                        0.10F);
                    ADDMSG(&f->msgs, msg_message("sailnolanding", "ship region", sh,
                        next_point));
                    damage_ship(sh, dmg);
                    /* we handle destruction at the end */
                }
                break;
            }

            if (curse_active(get_curse(next_point->attribs, ct_find("maelstrom")))) {
                if (do_maelstrom(next_point, u) == NULL)
                    break;
            }

        }

        /* !flying_ship */
        /* Falls Blockade, endet die Seglerei hier */
        if (move_blocked(u, current_point, next_point)) {
            ADDMSG(&u->faction->msgs, msg_message("sailfail", "ship region", sh,
                current_point));
            break;
        }

        /* Falls kein Problem, eines weiter ziehen */
        fset(sh, SF_MOVED);
        if (iroute) {
            add_regionlist(iroute, next_point);
            iroute = &(*iroute)->next;
        }
        step++;

        last_point = current_point;
        current_point = next_point;

        if (!fval(current_point->terrain, SEA_REGION)
            && !is_cursed(sh->attribs, C_SHIP_FLYING, 0))
            break;
        token = getstrtoken();
        error = movewhere(u, token, current_point, &next_point);
        if (error || next_point == NULL) {
            message *msg = movement_error(u, token, ord, error);
            if (msg != NULL) {
                add_message(&u->faction->msgs, msg);
                msg_release(msg);
            }
            next_point = current_point;
            break;
        }
    }

    if (sh->damage >= sh->size * DAMAGE_SCALE) {
        if (sh->region) {
            ADDMSG(&f->msgs, msg_message("shipsink", "ship", sh));
            remove_ship(&sh->region->ships, sh);
        }
        sh = NULL;
    }

    /* Nun enthält current_point die Region, in der das Schiff seine Runde
     * beendet hat. Wir generieren hier ein Ereignis für den Spieler, das
     * ihm sagt, bis wohin er gesegelt ist, falls er überhaupt vom Fleck
     * gekommen ist. Das ist nicht der Fall, wenn er von der Küste ins
     * Inland zu segeln versuchte */

    if (sh != NULL && fval(sh, SF_MOVED)) {
        unit *harbourmaster;
        /* nachdem alle Richtungen abgearbeitet wurden, und alle Einheiten
         * transferiert wurden, kann der aktuelle Befehl gelöscht werden. */
        cycle_route(ord, u, step);
        set_order(&u->thisorder, NULL);
        if (!move_on_land) {
            set_coast(sh, last_point, current_point);
        }

        if (is_cursed(sh->attribs, C_SHIP_FLYING, 0)) {
            ADDMSG(&f->msgs, msg_message("shipfly", "ship from to", sh,
                starting_point, current_point));
        }
        else {
            ADDMSG(&f->msgs, msg_message("shipsail", "ship from to", sh,
                starting_point, current_point));
        }

        /* Das Schiff und alle Einheiten darin werden nun von
         * starting_point nach current_point verschoben */

        /* Verfolgungen melden */
        if (fval(u, UFL_FOLLOWING))
            caught_target(current_point, u);

        sh = move_ship(sh, starting_point, current_point, *routep);

        /* Hafengebühren ? */

        harbourmaster = owner_buildingtyp(current_point, bt_find("harbour"));
        if (sh && harbourmaster != NULL) {
            item *itm;
            unit *u2;
            item *trans = NULL;

            for (u2 = current_point->units; u2; u2 = u2->next) {
                if (u2->ship == sh && !alliedunit(harbourmaster, u->faction, HELP_GUARD)) {

                    if (effskill(harbourmaster, SK_PERCEPTION) > effskill(u2, SK_STEALTH)) {
                        for (itm = u2->items; itm; itm = itm->next) {
                            const luxury_type *ltype = resource2luxury(itm->type->rtype);
                            if (ltype != NULL && itm->number > 0) {
                                int st = itm->number * effskill(harbourmaster, SK_TRADE) / 50;
                                st = _min(itm->number, st);

                                if (st > 0) {
                                    i_change(&u2->items, itm->type, -st);
                                    i_change(&harbourmaster->items, itm->type, st);
                                    i_add(&trans, i_new(itm->type, st));
                                }
                            }
                        }
                    }
                }
            }
            if (trans) {
                message *msg =
                    msg_message("harbor_trade", "unit items ship", harbourmaster, trans,
                    u->ship);
                add_message(&u->faction->msgs, msg);
                add_message(&harbourmaster->faction->msgs, msg);
                msg_release(msg);
                while (trans)
                    i_remove(&trans, trans);
            }
        }
    }
}

unit *get_captain(const ship * sh)
{
    const region *r = sh->region;
    unit *u;

    for (u = r->units; u; u = u->next) {
        if (u->ship == sh && u->number
            && eff_skill(u, SK_SAILING, r) >= sh->type->cptskill)
            return u;
    }

    return NULL;
}

/* Segeln, Wandern, Reiten
* when this routine returns a non-zero value, movement for the region needs
* to be done again because of followers that got new MOVE orders.
* Setting FL_LONGACTION will prevent a unit from being handled more than once
* by this routine
*
* the token parser needs to be initialized before calling this function!
*/

/** fleeing units use this function
*/
void run_to(unit * u, region * to)
{
    region_list *route = NULL;
    add_regionlist(&route, to);
    travel_route(u, route, NULL, NULL, TRAVEL_RUNNING);
    free_regionlist(route);
    /* weder transport noch follow */
}

static const region_list *travel_i(unit * u, const region_list * route_begin,
    const region_list * route_end, order * ord, int mode, follower ** followers)
{
    region *r = u->region;

    if (u->building && !can_leave(u)) {
        cmistake(u, u->thisorder, 150, MSG_MOVE);
        return route_begin;
    }
    switch (canwalk(u)) {
    case E_CANWALK_TOOHEAVY:
        cmistake(u, ord, 57, MSG_MOVE);
        return route_begin;
    case E_CANWALK_TOOMANYHORSES:
        cmistake(u, ord, 56, MSG_MOVE);
        return route_begin;
    case E_CANWALK_TOOMANYCARTS:
        cmistake(u, ord, 42, MSG_MOVE);
        return route_begin;
    }
    route_end = cap_route(r, route_begin, route_end, movement_speed(u));

    route_end = travel_route(u, route_begin, route_end, ord, mode);
    get_followers(u, r, route_end, followers);

    /* transportation */
    for (ord = u->orders; ord; ord = ord->next) {
        unit *ut = 0;

        if (getkeyword(ord) != K_TRANSPORT)
            continue;

        init_order(ord);
        if (getunit(r, u->faction, &ut) == GET_UNIT) {
            if (getkeyword(ut->thisorder) == K_DRIVE) {
                if (ut->building && !can_leave(ut)) {
                    cmistake(ut, ut->thisorder, 150, MSG_MOVE);
                    cmistake(u, ord, 99, MSG_MOVE);
                }
                else if (!can_move(ut)) {
                    cmistake(u, ord, 99, MSG_MOVE);
                }
                else {
                    bool found = false;

                    if (!fval(ut, UFL_NOTMOVING) && !LongHunger(ut)) {
                        unit *u2;
                        init_order(ut->thisorder);
                        getunit(u->region, ut->faction, &u2);
                        if (u2 == u) {
                            const region_list *route_to =
                                travel_route(ut, route_begin, route_end, ord,
                                TRAVEL_TRANSPORTED);

                            if (route_to != route_begin) {
                                get_followers(ut, r, route_to, followers);
                            }
                            ADDMSG(&ut->faction->msgs, msg_message("transport",
                                "unit target start end", u, ut, r, ut->region));
                            found = true;
                        }
                    }
                    if (!found) {
                        if (cansee(u->faction, u->region, ut, 0)) {
                            cmistake(u, ord, 90, MSG_MOVE);
                        }
                        else {
                            ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
                                "feedback_unit_not_found", ""));
                        }
                    }
                }
            }
        }
        else {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
                ""));
        }
    }
    return route_end;
}

/** traveling without ships
 * walking, flying or riding units use this function
 */
static void travel(unit * u, region_list ** routep)
{
    region *r = u->region;
    region_list *route_begin = NULL;
    follower *followers = NULL;

    if (routep)
        *routep = NULL;

    /* a few pre-checks that need not be done for each step: */
    if (!fval(r->terrain, SEA_REGION)) {
        ship *sh = u->ship;

        if (!can_leave(u)) {
            cmistake(u, u->thisorder, 150, MSG_MOVE);
            return;
        }

        /* An Land kein NACH wenn in dieser Runde Schiff VERLASSEN! */
        if (sh == NULL) {
            sh = leftship(u);
            if (sh && sh->region != u->region)
                sh = NULL;
        }
        if (sh) {
            unit *guard = is_guarded(r, u, GUARD_LANDING);
            if (guard) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
                    "region_guarded", "guard", guard));
                return;
            }
        }
        if (u->ship && u_race(u)->flags & RCF_SWIM) {
            cmistake(u, u->thisorder, 143, MSG_MOVE);
            return;
        }
    }
    else if (u->ship && fval(u->ship, SF_MOVED)) {
        /* die Einheit ist auf einem Schiff, das sich bereits bewegt hat */
        cmistake(u, u->thisorder, 13, MSG_MOVE);
        return;
    }

    make_route(u, u->thisorder, routep);
    route_begin = *routep;

    /* und ab die post: */
    travel_i(u, route_begin, NULL, u->thisorder, TRAVEL_NORMAL, &followers);

    /* followers */
    while (followers != NULL) {
        follower *fnext = followers->next;
        unit *uf = followers->uf;
        unit *ut = followers->ut;
        const region_list *route_end = followers->route_end;

        free(followers);
        followers = fnext;

        if (uf->region == r) {
            order *follow_order;
            const struct locale *lang = u->faction->locale;

            /* construct an order */
            follow_order = create_order(K_FOLLOW, lang, "%s %i",
                LOC(uf->faction->locale, parameters[P_UNIT]), ut->no);

            route_end = reroute(uf, route_begin, route_end);
            travel_i(uf, route_begin, route_end, follow_order, TRAVEL_FOLLOWING,
                &followers);
            caught_target(uf->region, uf);
            free_order(follow_order);
        }
    }

}

static void move(unit * u, bool move_on_land)
{
    region_list *route = NULL;

    assert(u->number);
    if (u->ship && u == ship_owner(u->ship)) {
        sail(u, u->thisorder, move_on_land, &route);
    }
    else {
        travel(u, &route);
    }

    fset(u, UFL_LONGACTION | UFL_NOTMOVING);
    set_order(&u->thisorder, NULL);

    if (route != NULL)
        free_regionlist(route);
}

typedef struct piracy_data {
    const struct faction *pirate;
    const struct faction *target;
    direction_t dir;
} piracy_data;

static void piracy_init(struct attrib *a)
{
    a->data.v = calloc(1, sizeof(piracy_data));
}

static void piracy_done(struct attrib *a)
{
    free(a->data.v);
}

static attrib_type at_piracy_direction = {
    "piracy_direction",
    piracy_init,
    piracy_done,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

static attrib *mk_piracy(const faction * pirate, const faction * target,
    direction_t target_dir)
{
    attrib *a = a_new(&at_piracy_direction);
    piracy_data *data = a->data.v;
    data->pirate = pirate;
    data->target = target;
    data->dir = target_dir;
    return a;
}

static void piracy_cmd(unit * u, struct order *ord)
{
    region *r = u->region;
    ship *sh = u->ship, *sh2;
    direction_t dir, target_dir = NODIRECTION;
    struct {
        const faction *target;
        int value;
    } aff[MAXDIRECTIONS];
    int saff = 0;
    int *il = NULL;
    const char *s;
    attrib *a;

    if (!sh) {
        cmistake(u, ord, 144, MSG_MOVE);
        return;
    }

    if (!u->ship || u != ship_owner(u->ship)) {
        cmistake(u, ord, 146, MSG_MOVE);
        return;
    }

    /* Feststellen, ob schon ein anderer alliierter Pirat ein
     * Ziel gefunden hat. */

    init_order(ord);
    s = getstrtoken();
    if (s != NULL && *s) {
        il = intlist_init();
        while (s && *s) {
            il = intlist_add(il, atoi36(s));
            s = getstrtoken();
        }
    }

    for (a = a_find(r->attribs, &at_piracy_direction);
        a && a->type == &at_piracy_direction; a = a->next) {
        piracy_data *data = a->data.v;
        const faction *p = data->pirate;
        const faction *t = data->target;

        if (alliedunit(u, p, HELP_FIGHT)) {
            if (il == 0 || (t && intlist_find(il, t->no))) {
                target_dir = data->dir;
                break;
            }
        }
    }

    /* Wenn nicht, sehen wir, ob wir ein Ziel finden. */

    if (target_dir == NODIRECTION) {
        /* Einheit ist also Kapitän. Jetzt gucken, in wievielen
         * Nachbarregionen potentielle Opfer sind. */

        for (dir = 0; dir < MAXDIRECTIONS; dir++) {
            region *rc = rconnect(r, dir);
            aff[dir].value = 0;
            aff[dir].target = 0;
            if (rc && fval(rc->terrain, SWIM_INTO) && check_takeoff(sh, r, rc)) {

                for (sh2 = rc->ships; sh2; sh2 = sh2->next) {
                    unit *cap = ship_owner(sh2);
                    if (cap) {
                        faction *f = visible_faction(cap->faction, cap);
                        if (alliedunit(u, f, HELP_FIGHT))
                            continue;
                        if (il == 0 || intlist_find(il, cap->faction->no)) {
                            ++aff[dir].value;
                            if (rng_int() % aff[dir].value == 0) {
                                aff[dir].target = f;
                            }
                        }
                    }
                }

                /* Und aufaddieren. */
                saff += aff[dir].value;
            }
        }

        if (saff != 0) {
            saff = rng_int() % saff;
            for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
                if (saff < aff[dir].value)
                    break;
                saff -= aff[dir].value;
            }
            target_dir = dir;
            a =
                a_add(&r->attribs, mk_piracy(u->faction, aff[dir].target, target_dir));
        }
    }

    free(il);

    /* Wenn kein Ziel gefunden, entsprechende Meldung generieren */
    if (target_dir == NODIRECTION) {
        ADDMSG(&u->faction->msgs, msg_message("piratenovictim",
            "ship region", sh, r));
        return;
    }

    /* Meldung generieren */
    ADDMSG(&u->faction->msgs, msg_message("piratesawvictim",
        "ship region dir", sh, r, target_dir));

    /* Befehl konstruieren */
    set_order(&u->thisorder, create_order(K_MOVE, u->faction->locale, "%s",
        LOC(u->faction->locale, directions[target_dir])));

    /* Bewegung ausführen */
    init_order(u->thisorder);
    move(u, true);
}

static void age_traveldir(region * r)
{
    attrib *a = a_find(r->attribs, &at_traveldir);

    while (a && a->type == &at_traveldir) {
        attrib *an = a->next;
        a->data.ca[3]--;
        if (a->data.ca[3] <= 0) {
            a_remove(&r->attribs, a);
        }
        a = an;
    }
}

static direction_t hunted_dir(attrib * at, int id)
{
    attrib *a = a_find(at, &at_shiptrail);
    direction_t d = NODIRECTION;

    while (a != NULL && a->type == &at_shiptrail) {
        traveldir *t = (traveldir *)(a->data.v);
        if (t->no == id) {
            d = t->dir;
            /* do not break, because we want the last one for this ship */
        }
        a = a->next;
    }

    return d;
}

static int hunt(unit * u, order * ord)
{
    region *rc = u->region;
    int bytes, moves, id, speed;
    char command[256], *bufp = command;
    size_t size = sizeof(command);
    direction_t dir;

    if (fval(u, UFL_NOTMOVING)) {
        return 0;
    }
    else if (!u->ship) {
        cmistake(u, ord, 144, MSG_MOVE);
        return 0;
    }
    else if (u != ship_owner(u->ship)) {
        cmistake(u, ord, 146, MSG_MOVE);
        return 0;
    }
    else if (!can_move(u)) {
        cmistake(u, ord, 55, MSG_MOVE);
        return 0;
    }

    id = getshipid();

    if (id <= 0) {
        cmistake(u, ord, 20, MSG_MOVE);
        return 0;
    }

    dir = hunted_dir(rc->attribs, id);

    if (dir == NODIRECTION) {
        ship *sh = findship(id);
        if (sh == NULL || sh->region != rc) {
            cmistake(u, ord, 20, MSG_MOVE);
        }
        return 0;
    }

    bufp = command;
    bytes = slprintf(bufp, size, "%s %s", LOC(u->faction->locale, keyword(K_MOVE)), LOC(u->faction->locale, directions[dir]));
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    moves = 1;

    speed = getuint();
    if (speed == 0) {
        speed = shipspeed(u->ship, u);
    }
    else {
        int maxspeed = shipspeed(u->ship, u);
        if (maxspeed < speed)
            speed = maxspeed;
    }
    rc = rconnect(rc, dir);
    while (moves < speed && (dir = hunted_dir(rc->attribs, id)) != NODIRECTION) {
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, LOC(u->faction->locale, directions[dir]), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        moves++;
        rc = rconnect(rc, dir);
    }

    /* In command steht jetzt das NACH-Kommando. */

    /* NACH ignorieren und Parsing initialisieren. */
    init_tokens_str(command);
    getstrtoken();
    /* NACH ausführen */
    move(u, false);
    return 1;                     /* true -> Einheitenliste von vorne durchgehen */
}

void destroy_damaged_ships(void)
{
    region *r;
    ship *sh, *shn;

    for (r = regions; r; r = r->next) {
        for (sh = r->ships; sh;) {
            shn = sh->next;
            if (sh->damage >= sh->size * DAMAGE_SCALE) {
                remove_ship(&sh->region->ships, sh);
            }
            sh = shn;
        }
    }
}

/* Bewegung, Verfolgung, Piraterie */

/** ships that folow other ships
 * Dann generieren die jagenden Einheiten ihre Befehle und
 * bewegen sich.
 * Following the trails of other ships.
 */
static void move_hunters(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        unit **up = &r->units;

        while (*up != NULL) {
            unit *u = *up;

            if (!fval(u, UFL_MOVED | UFL_NOTMOVING)) {
                order *ord;

                for (ord = u->orders; ord; ord = ord->next) {
                    if (getkeyword(ord) == K_FOLLOW) {
                        param_t p;

                        init_order(ord);
                        p = getparam(u->faction->locale);
                        if (p != P_SHIP) {
                            if (p != P_UNIT) {
                                cmistake(u, ord, 240, MSG_MOVE);
                            }
                            break;
                        }

                        /* wir folgen definitiv einem Schiff. */

                        if (fval(u, UFL_NOTMOVING)) {
                            cmistake(u, ord, 52, MSG_MOVE);
                            break;
                        }
                        else if (!can_move(u)) {
                            cmistake(u, ord, 55, MSG_MOVE);
                            break;
                        }

                        if (!fval(u, UFL_LONGACTION) && !LongHunger(u) && hunt(u, ord)) {
                            up = &r->units;
                            break;
                        }
                    }
                }
            }
            if (*up == u)
                up = &u->next;
        }
    }
}

/** Piraten and Drift
 *
 */
static void move_pirates(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        unit **up = &r->units;

        while (*up) {
            unit *u = *up;

            if (!fval(u, UFL_NOTMOVING) && getkeyword(u->thisorder) == K_PIRACY) {
                piracy_cmd(u, u->thisorder);
                fset(u, UFL_LONGACTION | UFL_NOTMOVING);
            }

            if (*up == u) {
                /* not moved, use next unit */
                up = &u->next;
            }
            else if (*up && (*up)->region != r) {
                /* moved the previous unit along with u (units on same ship)
                 * must start from the beginning again */
                up = &r->units;
            }
            /* else *up is already the next unit */
        }

        a_removeall(&r->attribs, &at_piracy_direction);
        age_traveldir(r);
    }
}

void movement(void)
{
    int ships;

    /* Initialize the additional encumbrance by transported units */
    init_transportation();

    /* Move ships in last phase, others first
     * This is to make sure you can't land someplace and then get off the ship
     * in the same turn.
     */
    for (ships = 0; ships <= 1; ++ships) {
        region *r = regions;
        while (r != NULL) {
            unit **up = &r->units;
            bool repeat = false;

            while (*up) {
                unit *u = *up;
                keyword_t kword;

                if (u->ship && fval(u->ship, SF_DRIFTED)) {
                    up = &u->next;
                    continue;
                }
                kword = getkeyword(u->thisorder);

                if (kword == K_ROUTE || kword == K_MOVE) {
                    /* after moving, the unit has no thisorder. this prevents
                     * it from moving twice (or getting error messages twice).
                     * UFL_NOTMOVING is set in combat if the unit is not allowed
                     * to move because it was involved in a battle.
                     */
                    if (fval(u, UFL_NOTMOVING)) {
                        if (fval(u, UFL_LONGACTION)) {
                            cmistake(u, u->thisorder, 52, MSG_MOVE);
                            set_order(&u->thisorder, NULL);
                        }
                        else {
                            cmistake(u, u->thisorder, 319, MSG_MOVE);
                            set_order(&u->thisorder, NULL);
                        }
                    }
                    else if (fval(u, UFL_MOVED)) {
                        cmistake(u, u->thisorder, 187, MSG_MOVE);
                        set_order(&u->thisorder, NULL);
                    }
                    else if (!can_move(u)) {
                        cmistake(u, u->thisorder, 55, MSG_MOVE);
                        set_order(&u->thisorder, NULL);
                    }
                    else {
                        if (ships) {
                            if (u->ship && ship_owner(u->ship) == u) {
                                init_order(u->thisorder);
                                move(u, false);
                            }
                        }
                        else {
                            if (!u->ship || ship_owner(u->ship) != u) {
                                init_order(u->thisorder);
                                move(u, false);
                            }
                        }
                    }
                }
                if (u->region == r) {
                    /* not moved, use next unit */
                    up = &u->next;
                }
                else {
                    if (*up && (*up)->region != r) {
                        /* moved the upcoming unit along with u (units on ships or followers,
                         * for example). must start from the beginning again immediately */
                        up = &r->units;
                        repeat = false;
                    }
                    else {
                        repeat = true;
                    }
                }
                /* else *up is already the next unit */
            }
            if (repeat)
                continue;
            if (ships == 0) {
                /* Abtreiben von beschädigten, unterbemannten, überladenen Schiffen */
                drifting_ships(r);
            }
            r = r->next;
        }
    }

    move_hunters();
    move_pirates();
}

/** Overrides long orders with a FOLLOW order if the target is moving.
 * BUGS: http://bugs.eressea.de/view.php?id=1444 (A folgt B folgt C)
 */
void follow_unit(unit * u)
{
    region *r = u->region;
    attrib *a = NULL;
    order *ord;
    unit *u2 = NULL;
    int followship = false;

    if (fval(u, UFL_NOTMOVING) || LongHunger(u))
        return;

    for (ord = u->orders; ord; ord = ord->next) {
        const struct locale *lang = u->faction->locale;

        if (getkeyword(ord) == K_FOLLOW) {
            int id;
            param_t p;
            init_order(ord);
            p = getparam(lang);
            if (p == P_UNIT) {
                id = read_unitid(u->faction, r);
                if (a != NULL) {
                    a = a_find(u->attribs, &at_follow);
                }

                if (id > 0) {
                    unit *uf = findunit(id);
                    if (!a) {
                        a = a_add(&u->attribs, make_follow(uf));
                    }
                    else {
                        a->data.v = uf;
                    }
                }
                else if (a) {
                    a_remove(&u->attribs, a);
                    a = NULL;
                }
            }
            else if (p == P_SHIP) {
                id = getshipid();
                if (id <= 0) {
                    /*	cmistake(u, ord, 20, MSG_MOVE); */
                }
                else  {
                    ship *sh = findship(id);
                    if (sh == NULL || (sh->region != r && hunted_dir(r->attribs, id) == NODIRECTION)) {
                        cmistake(u, ord, 20, MSG_MOVE);
                    }
                    else if (!u->ship) {
                        /*	cmistake(u, ord, 144, MSG_MOVE); */
                    }
                    else if (u != ship_owner(u->ship)) {
                        /*	cmistake(u, ord, 146, MSG_MOVE); */
                    }
                    else if (!can_move(u)) {
                        /*	cmistake(u, ord, 55, MSG_MOVE); */
                    }
                    else {
                        u2 = ship_owner(sh);
                        followship = true;
                    }
                }
            }
        }
    }

    if ((a || followship) && !fval(u, UFL_MOVED | UFL_NOTMOVING)) {
        bool follow = false;
        if (!followship) {
            u2 = a->data.v;
        }

        if (!u2 || (!followship && (u2->region != r || !cansee(u->faction, r, u2, 0)))) {
            return;
        }

        switch (getkeyword(u2->thisorder)) {
        case K_MOVE:
        case K_ROUTE:
            follow = true;
            break;
        case K_DRIVE:
            if (!followship) {
                follow = true;
            }
            break;
        default:
            for (ord = u2->orders; ord; ord = ord->next) {
                switch (getkeyword(ord)) {
                case K_FOLLOW:
                    follow = true;
                    break;
                case K_PIRACY:
                    if (followship) {
                        follow = true;
                    }
                    break;
                default:
                    if (followship && u2->region != r) {
                        follow = true;
                        break;
                    }
                    continue;
                }
                break;
            }
            break;
        }
        if (!follow) {
            attrib *a2 = a_find(u2->attribs, &at_follow);
            if (a2 != NULL) {
                unit *u3 = a2->data.v;
                follow = (u3 && u2->region == u3->region);
            }
        }
        if (follow) {
            fset(u, UFL_FOLLOWING);
            fset(u2, UFL_FOLLOWED);
            /* FOLLOW unit on a (potentially) moving unit prevents long orders */
            set_order(&u->thisorder, NULL);
        }
    }
}
