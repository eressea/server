/*
 *	Eressea PB(E)M host Copyright (C) 1998-2015
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schr�der
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <platform.h>
#include <kernel/config.h>

#include "monsters.h"

#include "economy.h"
#include "chaos.h"
#include "give.h"
#include "guard.h"
#include "laws.h"
#include "keyword.h"
#include "study.h"

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/hate.h>

#include <spells/regioncurse.h>

/* kernel includes */
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pathfinder.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <move.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>

#include <selist.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DRAGON_RANGE       20  /* max. Distanz zum n�chsten Drachenziel */
#define MOVE_PERCENT       25  /* chance fuer bewegung */
#define MAXILLUSION_TEXTS   3

static double attack_chance; /* rules.monsters.attack_chance, or default 0.4 */

static void give_peasants(unit *u, const item_type *itype, int reduce) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s 0 %d %s", LOC(u->faction->locale, keyword(K_GIVE)), reduce, LOC(u->faction->locale, itype->rtype->_name));
    unit_addorder(u, parse_order(buf, u->faction->locale));
}

static double random_move_chance(void) {
    static int rule;
    static int config;
    if (config_changed(&config)) {
        rule = config_get_int("rules.monsters.random_move_percent", MOVE_PERCENT);
    }
    return rule * 0.01;
}

static void reduce_weight(unit * u)
{
    int capacity, weight = 0;
    item **itmp = &u->items;
    int horses = get_resource(u, get_resourcetype(R_HORSE));

    if (horses > 0) {
        horses = MIN(horses, (u->number * 2));
        change_resource(u, get_resourcetype(R_HORSE), -horses);
    }

    /* 0. ditch any vehicles */
    while (*itmp != NULL) {
        item *itm = *itmp;
        const item_type *itype = itm->type;
        if (itype->flags & ITF_VEHICLE) {
            give_peasants(u, itm->type, itm->number);
        }
        else {
            weight += itm->number * itype->weight;
        }
        if (*itmp == itm)
            itmp = &itm->next;
    }

    capacity = walkingcapacity(u);

    /* 1. get rid of anything that isn't silver or really lightweight or helpful in combat */
    for (itmp = &u->items; *itmp && capacity > 0;) {
        item *itm = *itmp;
        const item_type *itype = itm->type;
        weight += itm->number * itype->weight;
        if (weight > capacity) {
            if (itype->weight >= 10 && itype->rtype->wtype == 0
                && itype->rtype->atype == 0) {
                if (itype->capacity < itype->weight) {
                    int reduce = MIN(itm->number, -((capacity - weight) / itype->weight));
                    give_peasants(u, itm->type, reduce);
                    weight -= reduce * itype->weight;
                }
            }
        }
        if (*itmp == itm)
            itmp = &itm->next;
    }

    for (itmp = &u->items; *itmp && weight > capacity;) {
        item *itm = *itmp;
        const item_type *itype = itm->type;
        weight += itm->number * itype->weight;
        if (itype->capacity < itype->weight) {
            int reduce = MIN(itm->number, -((capacity - weight) / itype->weight));
            give_peasants(u, itm->type, reduce);
            weight -= reduce * itype->weight;
        }
        if (*itmp == itm)
            itmp = &itm->next;
    }
}

static order *monster_attack(unit * u, const unit * target)
{
    assert(u->region == target->region);
    assert(u->faction != target->faction);
    if (!cansee(u->faction, u->region, target, 0))
        return NULL;
    if (monster_is_waiting(u))
        return NULL;

    return create_order(K_ATTACK, u->faction->locale, "%i", target->no);
}

void monsters_desert(struct faction *monsters)
{
    region *r;

    assert(monsters!=NULL);
    for (r = regions; r; r = r->next) {
        unit *u;
        
        for (u = r->units; u; u = u->next) {
            if (u->faction!=monsters
                && (u_race(u)->flags & RCF_DESERT)) {
                if (fval(u, UFL_ISNEW))
                    continue;
                if (rng_int() % 100 < 5) {
                    ADDMSG(&u->faction->msgs, msg_message("desertion",
                        "unit region", u, r));
                    u_setfaction(u, monsters);
                }
            }
        }
    }
}

int monster_attacks(unit * monster, bool rich_only)
{
    const race *rc_serpent = get_race(RC_SEASERPENT);
    if (monster->status < ST_AVOID) {
        region *r = monster->region;
        unit *u2;
        int money = 0;

        for (u2 = r->units; u2; u2 = u2->next) {
            if (u2->faction != monster->faction && cansee(monster->faction, r, u2, 0) && !in_safe_building(u2, monster)) {
                int m = get_money(u2);
                if (u_race(monster) == rc_serpent) {
                    /* attack bigger ships only */
                    if (!u2->ship || u2->ship->type->cargo <= 50000) {
                        continue;
                    }
                }
                if (!rich_only || m > 0) {
                    order *ord = monster_attack(monster, u2);
                    if (ord) {
                        addlist(&monster->orders, ord);
                        money += m;
                    }
                }
            }
        }
        return money;
    }
    return 0;
}

static order *get_money_for_dragon(region * r, unit * udragon, int wanted)
{
    int money;
    bool attacks = attack_chance > 0.0;

    /* falls genug geld in der region ist, treiben wir steuern ein. */
    if (rmoney(r) >= wanted) {
        /* 5% chance, dass der drache aus einer laune raus attackiert */
        if (!attacks) {
            /* Drachen haben in E3 und E4 keine Einnahmen. Neuer Befehl Pluendern erstmal nur fuer Monster?*/
            return create_order(K_LOOT, default_locale, NULL);
        }
    }

    /* falls der drache launisch ist, oder das regionssilber knapp, greift er alle an
     * und holt sich Silber von Einheiten, vorausgesetzt er bewacht bereits */
    money = 0;
    if (attacks && is_guard(udragon)) {
        money += monster_attacks(udragon, true);
    }

    /* falls die einnahmen erreicht werden, bleibt das monster noch eine */
    /* runde hier. */
    if (money + rmoney(r) >= wanted) {
        return create_order(K_LOOT, default_locale, NULL);
    }

    /* wenn wir NULL zurueckliefern, macht der drache was anderes, z.b. weggehen */
    return NULL;
}

static int all_money(region * r, faction * f)
{
    unit *u;
    int m;

    m = rmoney(r);
    for (u = r->units; u; u = u->next) {
        if (f != u->faction) {
            m += get_money(u);
        }
    }
    return m;
}

static direction_t richest_neighbour(region * r, faction * f, int absolut)
{

    /* m - maximum an Geld, d - Richtung, i - index, t = Geld hier */

    double m;
    double t;
    direction_t d = NODIRECTION, i;

    if (absolut == 1 || rpeasants(r) == 0) {
        m = (double)all_money(r, f);
    }
    else {
        m = (double)all_money(r, f) / (double)rpeasants(r);
    }

    /* finde die region mit dem meisten geld */

    for (i = 0; i != MAXDIRECTIONS; i++) {
        region *rn = rconnect(r, i);
        if (rn != NULL && fval(rn->terrain, LAND_REGION)) {
            if (absolut == 1 || rpeasants(rn) == 0) {
                t = (double)all_money(rn, f);
            }
            else {
                t = (double)all_money(rn, f) / (double)rpeasants(rn);
            }

            if (t > m) {
                m = t;
                d = i;
            }
        }
    }
    return d;
}

static bool room_for_race_in_region(region * r, const race * rc)
{
    if (rc->splitsize > 0) {
        unit *u;
        int c = 0;

        for (u = r->units; u; u = u->next) {
            if (u_race(u) == rc) {
                c += u->number;
                if (c > rc->splitsize * 2) {
                    return false;
                }
            }
        }
    }
    return true;
}

static direction_t random_neighbour(region * r, unit * u)
{
    int i;
    region *next[MAXDIRECTIONS], *backup[MAXDIRECTIONS];
    region **pick;
    int rr, c = 0, c2 = 0;
    const race *rc = u_race(u);

    get_neighbours(r, next);
    /* Nachsehen, wieviele Regionen in Frage kommen */

    for (i = 0; i != MAXDIRECTIONS; i++) {
        region *rn = next[i];
        if (rn && can_survive(u, rn)) {
            if (room_for_race_in_region(rn, rc)) {
                c++;
            } else {
                next[i] = NULL;
            }
            backup[i] = rn;
            c2++;
        } else {
            next[i] = NULL;
            backup[i] = NULL;
        }
    }

    pick = next;
    if (c == 0) {
        if (c2 == 0) {
            return NODIRECTION;
        }
        else {
            pick = backup;
            c = c2;
        }
    }

    /* Zuf�llig eine ausw�hlen */

    rr = rng_int() % c;

    /* Durchz�hlen */

    c = 0;
    for (i = 0; i != MAXDIRECTIONS; i++) {
        region *rn = pick[i];
        if (rn) {
            if (c == rr) {
                return (direction_t)i;
            }
            c++;
        }
    }

    assert(1 == 0);               /* Bis hierhin sollte er niemals kommen. */
    return NODIRECTION;
}

static direction_t treeman_neighbour(region * r)
{
    int i;
    int rr;
    int c = 0;
    region * next[MAXDIRECTIONS];

    get_neighbours(r, next);
    /* Nachsehen, wieviele Regionen in Frage kommen */

    for (i = 0; i != MAXDIRECTIONS; i++) {
        if (next[i] && rterrain(next[i]) != T_OCEAN
            && rterrain(next[i]) != T_GLACIER && rterrain(next[i]) != T_DESERT) {
            ++c;
        }
    }

    if (c == 0) {
        return NODIRECTION;
    }
    /* Zuf�llig eine ausw�hlen */

    rr = rng_int() % c;

    /* Durchz�hlen */

    c = -1;
    for (i = 0; i != MAXDIRECTIONS; i++) {
        if (next[i] && rterrain(next[i]) != T_OCEAN
            && rterrain(next[i]) != T_GLACIER && rterrain(next[i]) != T_DESERT) {
            if (++c == rr) {
                return (direction_t)i;
            }
        }
    }

    assert(!"this should never happen");               /* Bis hierhin sollte er niemals kommen. */
    return NODIRECTION;
}

static order *monster_move(region * r, unit * u)
{
    direction_t d = NODIRECTION;

    if (monster_is_waiting(u)) {
        return NULL;
    }
    if (fval(u_race(u), RCF_DRAGON)) {
        d = richest_neighbour(r, u->faction, 1);
    }
    else if (get_race(RC_TREEMAN)==u_race(u)) {
        d = treeman_neighbour(r);
    }
    else {
        d = random_neighbour(r, u);
    }
    /* falls kein geld gefunden wird, zufaellig verreisen, aber nicht in
     * den ozean */

    if (d == NODIRECTION)
        return NULL;

    reduce_weight(u);
    return create_order(K_MOVE, u->faction->locale, "%s",
        LOC(u->faction->locale, directions[d]));
}

static int dragon_affinity_value(region * r, unit * u)
{
    int m = all_money(r, u->faction);

    if (u_race(u) == get_race(RC_FIREDRAGON)) {
        return dice(4, m / 2);
    }
    else {
        return dice(6, m / 3);
    }
}

static attrib *set_new_dragon_target(unit * u, region * r, int range)
{
    int max_affinity = 0;
    region *max_region = NULL;
    selist *ql, *rlist = regions_in_range(r, range, allowed_dragon);
    int qi;

    for (qi = 0, ql = rlist; ql; selist_advance(&ql, &qi, 1)) {
        region *r2 = (region *)selist_get(ql, qi);
        int affinity = dragon_affinity_value(r2, u);
        if (affinity > max_affinity) {
            max_affinity = affinity;
            max_region = r2;
        }
    }

    selist_free(rlist);

    if (max_region && max_region != r) {
        attrib *a = a_find(u->attribs, &at_targetregion);
        if (!a) {
            a = a_add(&u->attribs, make_targetregion(max_region));
        }
        else {
            a->data.v = max_region;
        }
        return a;
    }
    return NULL;
}

static order *make_movement_order(unit * u, const region * target, int moves,
    bool(*allowed) (const region *, const region *))
{
    region *r = u->region;
    region **plan;
    int bytes, position = 0;
    char zOrder[128], *bufp = zOrder;
    size_t size = sizeof(zOrder) - 1;

    if (monster_is_waiting(u))
        return NULL;

    plan = path_find(r, target, DRAGON_RANGE * 5, allowed);
    if (plan == NULL)
        return NULL;

    while (position != moves && plan[position + 1]) {
        region *prev = plan[position];
        region *next = plan[++position];
        direction_t dir = reldirection(prev, next);
        assert(dir != NODIRECTION && dir != D_SPECIAL);
        if (size > 1 && bufp != zOrder) {
            *bufp++ = ' ';
            --size;
        }
        bytes =
            (int)strlcpy(bufp,
            (const char *)LOC(u->faction->locale, directions[dir]), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }

    *bufp = 0;
    return create_order(K_MOVE, u->faction->locale, zOrder);
}

void random_growl(const unit *u, region *target, int rand)
{
    const struct locale *lang = u->faction->locale;
    const char *growl;
    switch(rand){
    case 1: growl = "growl1"; break;
    case 2: growl = "growl2"; break;
    case 3: growl = "growl3"; break;
    case 4: growl = "growl4"; break;
    default: growl = "growl0";
    }


    if (rname(target, lang)) {
        message *msg = msg_message("dragon_growl", "dragon number target growl", u, u->number, target, growl);
        ADDMSG(&u->region->msgs, msg);
    }
}

extern struct attrib_type at_direction;

static order *monster_learn(unit * u)
{
    int c = 0;
    int n;
    skill *sv;
    const struct locale *lang = u->faction->locale;

    /* can these monsters even study? */
    if (!unit_can_study(u)) {
        return NULL;
    }

    /* Monster lernt ein zuf�lliges Talent aus allen, in denen es schon
     * Lerntage hat. */
    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
        if (sv->level > 0)
            ++c;
    }

    if (c == 0)
        return NULL;

    n = rng_int() % c + 1;
    c = 0;

    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
        if (sv->level > 0) {
            if (++c == n) {
                return create_order(K_STUDY, lang, "'%s'", skillname(sv->id, lang));
            }
        }
    }
    return NULL;
}

static bool check_overpopulated(const unit * u)
{
    const race *rc = u_race(u);
    if (rc->splitsize > 0) {
        unit *u2;
        int c = 0;

        for (u2 = u->region->units; u2; u2 = u2->next) {
            if (u != u2 && u_race(u2) == rc) {
                c += u2->number;
                if (c > rc->splitsize * 2)
                    return true;
            }
        }
    }
    return false;
}

static void recruit_dracoids(unit * dragon, int size)
{
    faction *f = dragon->faction;
    region *r = dragon->region;
    const struct item *weapon = NULL;
    unit *un = create_unit(r, f, size, get_race(RC_DRACOID), 0, NULL, NULL);

    fset(un, UFL_ISNEW | UFL_MOVED);

    name_unit(un);
    change_money(dragon, -un->number * 50);
    equip_unit(un, get_equipment("new_dracoid"));

    setstatus(un, ST_FIGHT);
    for (weapon = un->items; weapon; weapon = weapon->next) {
        const weapon_type *wtype = weapon->type->rtype->wtype;
        if (wtype && wtype->flags & WTF_MISSILE) {
            setstatus(un, ST_BEHIND);
            break;
        }
    }
}

static order *plan_dragon(unit * u)
{
    attrib *ta = a_find(u->attribs, &at_targetregion);
    region *r = u->region;
    region *tr = NULL;
    bool move = false;
    order *long_order = NULL;
    static int rc_cache;
    static const race *rc_wyrm;
    const race * rc = u_race(u);

    if (rc_changed(&rc_cache)) {
        rc_wyrm = get_race(RC_WYRM);
    }

    if (ta == NULL) {
        move |= (rpeasants(r) == 0);   /* when no peasants, move */
        move |= (rmoney(r) == 0);      /* when no money, move */
    }
    move |= chance(0.04);         /* 4% chance to change your mind */

    if (rc == rc_wyrm && !move) {
        unit *u2;
        for (u2 = r->units; u2; u2 = u2->next) {
            /* wyrme sind einzelg�nger */
            if (u2 == u) {
                /* we do not make room for newcomers, so we don't need to look at them */
                break;
            }
            if (u2 != u && u_race(u2) == u_race(u) && chance(0.5)) {
                move = true;
                break;
            }
        }
    }

    if (move && (!ta || chance(0.1))) {
        /* dragon gets bored and looks for a different place to go */
        ta = set_new_dragon_target(u, u->region, DRAGON_RANGE);
    }
    if (ta != NULL) {
        tr = (region *)ta->data.v;
        if (tr == NULL || !path_exists(u->region, tr, DRAGON_RANGE, allowed_dragon)) {
            ta = set_new_dragon_target(u, u->region, DRAGON_RANGE);
            if (ta) {
                tr = findregion(ta->data.sa[0], ta->data.sa[1]);
            }
        }
    }
    if (tr != NULL) {
        assert(long_order == NULL);
        /* TODO: per-race planning functions? */
        if (rc == rc_wyrm) {
            long_order = make_movement_order(u, tr, 1, allowed_dragon);
        }
        else {
            switch (old_race(rc)) {
            case RC_FIREDRAGON:
                long_order = make_movement_order(u, tr, 4, allowed_dragon);
                break;
            case RC_DRAGON:
                long_order = make_movement_order(u, tr, 3, allowed_dragon);
                break;
            default:
                break;
            }
        }
        if (long_order) {
            reduce_weight(u);
        }
        if (rng_int() % 100 < 15) {
            random_growl(u, tr, rng_int() % 5);
        }
    }
    else {
        /* we have no target. do we like it here, then? */
        long_order = get_money_for_dragon(u->region, u, income(u));
        if (long_order == NULL) {
            /* money is gone, need a new target */
            set_new_dragon_target(u, u->region, DRAGON_RANGE);
        }
        else if (u_race(u) != get_race(RC_FIREDRAGON)) {
            /* neue dracoiden! */
            if (r->land && !fval(r->terrain, FORBIDDEN_REGION)) {
                int ra = 20 + rng_int() % 100;
                if (get_money(u) > ra * 50 + 100 && rng_int() % 100 < 50) {
                    recruit_dracoids(u, ra);
                }
            }
        }
    }
    if (long_order == NULL) {
        int attempts = 0;
        skill_t sk = SK_PERCEPTION;
        /* study perception (or a random useful skill) */
        while (!skill_enabled(sk) || (attempts < MAXSKILLS && u_race(u)->bonus[sk] < (++attempts < 10?1:-5 ))) {
            sk = (skill_t)(rng_int() % MAXSKILLS);
        }
        long_order = create_order(K_STUDY, u->faction->locale, "'%s'",
            skillname(sk, u->faction->locale));
    }
    return long_order;
}

void plan_monsters(faction * f)
{
    region *r;
    
    assert(f);
    attack_chance = config_get_flt("rules.monsters.attack_chance", 0.4);
    f->lastorders = turn;

    for (r = regions; r; r = r->next) {
        unit *u;
        bool attacking = chance(attack_chance);

        for (u = r->units; u; u = u->next) {
            const race *rc = u_race(u);
            attrib *ta;
            order *long_order = NULL;

            /* Ab hier nur noch Befehle f�r NPC-Einheiten. */
            if (u->faction!=f)
                continue;

            /* Befehle m�ssen jede Runde neu gegeben werden: */
            free_orders(&u->orders);
            if (skill_enabled(SK_PERCEPTION)) {
                /* Monster bekommen jede Runde ein paar Tage Wahrnehmung dazu */
                produceexp(u, SK_PERCEPTION, u->number);
            }

            if (attacking && (!r->land || is_guard(u))) {
                monster_attacks(u, false);
            }

            /* units with a plan to kill get ATTACK orders: */
            ta = a_find(u->attribs, &at_hate);
            if (ta && !monster_is_waiting(u)) {
                unit *tu = (unit *)ta->data.v;
                if (tu && tu->region == r) {
                    order * ord = monster_attack(u, tu);
                    if (ord) {
                        addlist(&u->orders, ord);
                    }
                }
                else if (tu) {
                    tu = findunitg(ta->data.i, NULL);
                    if (tu != NULL) {
                        long_order = make_movement_order(u, tu->region, 2, allowed_walk);
                    }
                }
                else
                    a_remove(&u->attribs, ta);
            }

            /* All monsters guard the region: */
            if (u->status < ST_FLEE && !monster_is_waiting(u) && r->land) {
                addlist(&u->orders, create_order(K_GUARD, u->faction->locale, NULL));
            }

            /* Einheiten mit Bewegungsplan kriegen ein NACH: */
            if (long_order == NULL) {
                ta = a_find(u->attribs, &at_targetregion);
                if (ta) {
                    if (u->region == (region *)ta->data.v) {
                        a_remove(&u->attribs, ta);
                    }
                }
                else if (rc->flags & RCF_MOVERANDOM) {
                    if (chance(random_move_chance()) || check_overpopulated(u)) {
                        long_order = monster_move(r, u);
                    }
                }
            }

            if (long_order == NULL) {
                /* Ab hier noch nicht generalisierte Spezialbehandlungen. */

                if (fval(rc, RCF_DRAGON)) {
                    long_order = plan_dragon(u);
                }
                else {
                    if (rc == get_race(RC_SEASERPENT)) {
                        long_order = create_order(K_PIRACY, f->locale, NULL);
                    }
                    else {
                        if (rc->flags & RCF_LEARN) {
                            long_order = monster_learn(u);
                        }
                    }
                }
            }
            if (long_order == NULL && unit_can_study(u)) {
                /* Einheiten, die Waffenlosen Kampf lernen k�nnten, lernen es um
                * zu bewachen: */
                if (rc->bonus[SK_WEAPONLESS] != -99) {
                    if (effskill(u, SK_WEAPONLESS, 0) < 1) {
                        long_order =
                            create_order(K_STUDY, f->locale, "'%s'",
                                skillname(SK_WEAPONLESS, f->locale));
                    }
                }
            }

            if (long_order) {
                addlist(&u->orders, long_order);
            }
        }
    }
    pathfinder_cleanup();
}

static double chaosfactor(region * r)
{
    return fval(r, RF_CHAOTIC) ? ((double)(1 + get_chaoscount(r)) / 1000.0) : 0.0;
}

static int nrand(int start, int sub)
{
    int res = 0;

    do {
        if (rng_int() % 100 < start)
            res++;
        start -= sub;
    } while (start > 0);

    return res;
}

unit *spawn_seaserpent(region *r, faction *f) {
    unit *u = create_unit(r, f, 1, get_race(RC_SEASERPENT), 0, NULL, NULL);
    fset(u, UFL_ISNEW | UFL_MOVED);
    equip_unit(u, get_equipment("seed_seaserpent"));
    return u;
}

/** 
 * Drachen und Seeschlangen k�nnen entstehen 
 */
void spawn_dragons(void)
{
    region *r;
    faction *monsters = get_or_create_monsters();
    int minage = config_get_int("monsters.spawn.min_age", 100);
    int spawn_chance = 100 * config_get_int("monsters.spawn.chance", 100);

    if (spawn_chance <= 0) {
        /* monster spawning disabled */
        return;
    }
    for (r = regions; r; r = r->next) {
        unit *u;
        if (r->age < minage) {
            continue;
        }
        if (fval(r->terrain, SEA_REGION)) {
            if (rng_int() % spawn_chance < 1) {
                u = spawn_seaserpent(r, monsters);
            }
        }
        else if ((r->terrain == newterrain(T_GLACIER)
            || r->terrain == newterrain(T_SWAMP)
            || r->terrain == newterrain(T_DESERT))
            && rng_int() % spawn_chance < (5 + 100 * chaosfactor(r))) {
            if (chance(0.80)) {
                u = create_unit(r, monsters, nrand(60, 20) + 1, get_race(RC_FIREDRAGON), 0, NULL, NULL);
            }
            else {
                u = create_unit(r, monsters, nrand(30, 20) + 1, get_race(RC_DRAGON), 0, NULL, NULL);
            }
            fset(u, UFL_ISNEW | UFL_MOVED);
            equip_unit(u, get_equipment("seed_dragon"));

            log_debug("spawning %d %s in %s.\n", u->number,
                LOC(default_locale,
                rc_name_s(u_race(u), (u->number == 1) ? NAME_SINGULAR : NAME_PLURAL)), regionname(r, NULL));

            name_unit(u);

            /* add message to the region */
            ADDMSG(&r->msgs,
                msg_message("sighting", "region race number", r, u_race(u), u->number));
        }
    }
}

/** Untote k�nnen entstehen */
void spawn_undead(void)
{
    region *r;
    faction *monsters = get_monsters();

    for (r = regions; r; r = r->next) {
        int unburied = deathcount(r);

        if (r->attribs) {
            if (curse_active(get_curse(r->attribs, &ct_holyground))) {
                continue;
            }
        }
        /* Chance 0.1% * chaosfactor */
        if (r->land && unburied > rpeasants(r) / 20
            && rng_int() % 10000 < (100 + 100 * chaosfactor(r))) {
            message *msg;
            unit *u;
            /* es ist sinnfrei, wenn irgendwo im Wald 3er-Einheiten Untote entstehen.
             * Lieber sammeln lassen, bis sie mindestens 5% der Bev�lkerung sind, und
             * dann erst auferstehen. */
            int undead = unburied / (rng_int() % 2 + 1);
            const race *rc = NULL;
            int i;
            if (r->age < 100)
                undead = undead * r->age / 100; /* newbie-regionen kriegen weniger ab */

            if (!undead || r->age < 20)
                continue;

            switch (rng_int() % 3) {
            case 0:
                rc = get_race(RC_SKELETON);
                break;
            case 1:
                rc = get_race(RC_ZOMBIE);
                break;
            default:
                rc = get_race(RC_GHOUL);
                break;
            }

            u = create_unit(r, monsters, undead, rc, 0, NULL, NULL);
            fset(u, UFL_ISNEW | UFL_MOVED);
            if ((rc == get_race(RC_SKELETON) || rc == get_race(RC_ZOMBIE))
                && rng_int() % 10 < 4) {
                equip_unit(u, get_equipment("rising_undead"));
            }

            for (i = 0; i < MAXSKILLS; i++) {
                if (rc->bonus[i] >= 1) {
                    set_level(u, (skill_t)i, 1);
                }
            }
            u->hp = unit_max_hp(u) * u->number;

            deathcounts(r, -undead);
            name_unit(u);

            log_debug("spawning %d %s in %s.\n", u->number,
                LOC(default_locale,
                rc_name_s(u_race(u), (u->number == 1) ? NAME_SINGULAR : NAME_PLURAL)), regionname(r, NULL));
          msg = msg_message("undeadrise", "region", r);
          add_message(&r->msgs, msg);
          for (u = r->units; u; u = u->next)
              freset(u->faction, FFL_SELECT);
          for (u = r->units; u; u = u->next) {
              if (fval(u->faction, FFL_SELECT))
                  continue;
              fset(u->faction, FFL_SELECT);
              add_message(&u->faction->msgs, msg);
          }
          msg_release(msg);
        }
        else {
            int i = deathcount(r);
            if (i) {
                /* Gr�ber verwittern, 3% der Untoten finden die ewige Ruhe */
                deathcounts(r, (int)(-i * 0.03));
            }
        }
    }
}

bool monster_is_waiting(const unit * u)
{
    int test = fval(u_race(u), RCF_ATTACK_MOVED) ? UFL_ISNEW : UFL_ISNEW | UFL_MOVED;
    if (fval(u, test))
        return true;
    return false;
}

static void eaten_by_monster(unit * u)
{
    /* adjustment for smaller worlds */
    double multi = RESOURCE_QUANTITY * newterrain(T_PLAIN)->size / 10000.0;
    int n = 0;
    int horse = -1;
    const resource_type *rhorse = get_resourcetype(R_HORSE);
    const race *rc = u_race(u);
    int scare;

    scare = rc_scare(rc);
    if (scare>0) {
        n = rng_int() % scare * u->number;
    } else {
        n = rng_int() % (u->number / 20 + 1);
        horse = 0;
    }
    horse = horse ? i_get(u->items, rhorse->itype) : 0;

    n = (int)(n * multi);
    if (n > 0) {
        n = lovar(n);
        n = MIN(rpeasants(u->region), n);

        if (n > 0) {
            deathcounts(u->region, n);
            rsetpeasants(u->region, rpeasants(u->region) - n);
            ADDMSG(&u->region->msgs, msg_message("eatpeasants", "unit amount", u, n));
        }
    }
    if (horse > 0) {
        i_change(&u->items, rhorse->itype, -horse);
        ADDMSG(&u->region->msgs, msg_message("eathorse", "unit amount", u, horse));
    }
}

static void absorbed_by_monster(unit * u)
{
    int n = rng_int() % (u->number / 20 + 1);

    if (n > 0) {
        n = lovar(n);
        n = MIN(rpeasants(u->region), n);
        if (n > 0) {
            rsetpeasants(u->region, rpeasants(u->region) - n);
            scale_number(u, u->number + n);
            ADDMSG(&u->region->msgs, msg_message("absorbpeasants",
                "unit race amount", u, u_race(u), n));
        }
    }
}

static int scareaway(region * r, int anzahl)
{
    int n, p, diff = 0, emigrants[MAXDIRECTIONS];
    direction_t d;

    anzahl = MIN(MAX(1, anzahl), rpeasants(r));

    /* Wandern am Ende der Woche (normal) oder wegen Monster. Die
     * Wanderung wird erst am Ende von demographics () ausgefuehrt.
     * emigrants[] ist local, weil r->newpeasants durch die Monster
     * vielleicht schon hochgezaehlt worden ist. */

    for (d = 0; d != MAXDIRECTIONS; d++)
        emigrants[d] = 0;

    p = rpeasants(r);
    assert(p >= 0 && anzahl >= 0);
    for (n = MIN(p, anzahl); n; n--) {
        direction_t dir = (direction_t)(rng_int() % MAXDIRECTIONS);
        region *rc = rconnect(r, dir);

        if (rc && fval(rc->terrain, LAND_REGION)) {
            ++diff;
            rc->land->newpeasants++;
            emigrants[dir]++;
        }
    }
    rsetpeasants(r, p - diff);
    assert(p >= diff);
    return diff;
}

static void scared_by_monster(unit * u)
{
    int n;
    const race *rc = u_race(u);
    int scare;
    
    scare = rc_scare(rc);
    if (scare>0) {
        n = rng_int() % scare * u->number;
    } else {
        n = rng_int() % (u->number / 4 + 1);
    }
    if (n > 0) {
        n = lovar(n);
        n = MIN(rpeasants(u->region), n);
        if (n > 0) {
            n = scareaway(u->region, n);
            if (n > 0) {
                ADDMSG(&u->region->msgs, msg_message("fleescared",
                    "amount unit", n, u));
            }
        }
    }
}

void monster_kills_peasants(unit * u)
{
    if (!monster_is_waiting(u)) {
        if (u_race(u)->flags & RCF_SCAREPEASANTS) {
            scared_by_monster(u);
        }
        if (u_race(u)->flags & RCF_KILLPEASANTS) {
            eaten_by_monster(u);
        }
        if (u_race(u)->flags & RCF_ABSORBPEASANTS) {
            absorbed_by_monster(u);
        }
    }
}

void make_zombie(unit * u)
{
    u_setfaction(u, get_monsters());
    scale_number(u, 1);
    u->hp = unit_max_hp(u) * u->number;
    u_setrace(u, get_race(RC_ZOMBIE));
    u->irace = NULL;
}
