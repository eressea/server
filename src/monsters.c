#include "monsters.h"

#include "battle.h"
#include "economy.h"
#include "guard.h"
#include "give.h"
#include "laws.h"
#include "move.h"
#include "study.h"

/* kernel includes */
#include "kernel/attrib.h"
#include "kernel/build.h"
#include "kernel/building.h"
#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/curse.h"
#include "kernel/equipment.h"
#include "kernel/event.h"
#include "kernel/faction.h"
#include "kernel/group.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/pathfinder.h"
#include "kernel/pool.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/ship.h"
#include "kernel/skills.h"
#include "kernel/terrain.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"

/* util includes */
#include "util/base36.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/log.h"
#include "util/message.h"
#include "util/stats.h"
#include "util/rand.h"
#include "util/rng.h"

/* attributes includes */
#include "attributes/hate.h"
#include "attributes/otherfaction.h"
#include "attributes/stealth.h"
#include "attributes/targetregion.h"

#include "spells/regioncurse.h"

#include <selist.h>
#include <strings.h>

#include <stb_ds.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DRAGON_RANGE       20  /* max. Distanz zum naechsten Drachenziel */
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
    item * itm;

    /* 0. calculate weight, without animals & vehicles */
    for (itm = u->items; itm; itm = itm->next) {
        const item_type *itype = itm->type;
        if (itype->flags & (ITF_VEHICLE|ITF_ANIMAL)) {
            give_peasants(u, itm->type, itm->number);
        }
        else {
            weight += itm->number * itype->weight;
        }
    }

    capacity = walkingcapacity(u, NULL);

    /* 1. get rid of anything that isn't silver or really lightweight or helpful in combat */
    for (itm = u->items; itm && capacity > 0; itm = itm->next) {
        const item_type *itype = itm->type;
        weight += itm->number * itype->weight;
        if (weight > capacity) {
            if (itype->weight >= 10 && itype->rtype->wtype == 0
                && itype->rtype->atype == 0) {
                if (itype->capacity < itype->weight) {
                    int reduce = (weight - capacity) / itype->weight;
                    if (reduce > itm->number) {
                        reduce = itm->number;
                    }
                    give_peasants(u, itm->type, reduce);
                    weight -= reduce * itype->weight;
                }
            }
        }
    }

    for (itm = u->items; itm && weight > capacity; itm = itm->next) {
        const item_type *itype = itm->type;
        weight += itm->number * itype->weight;
        if (itype->capacity < itype->weight) {
            int reduce = (weight - capacity) / itype->weight;
            if (reduce > itm->number) {
                reduce = itm->number;
            }
            give_peasants(u, itm->type, reduce);
            weight -= reduce * itype->weight;
        }
    }
}

static bool monster_is_waiting(const unit * u)
{
    int test = fval(u_race(u), RCF_ATTACK_MOVED) ? UFL_ISNEW : (UFL_ISNEW | UFL_MOVED);
    return fval(u, test) != 0;
}

static bool monster_can_attack(const unit * u)
{
    if (u->status >= ST_AVOID) {
        return false;
    }
    if (u->region->land) {
        return is_guard(u);
    }
    else if (fval(u->region->terrain, SEA_REGION)) {
        return fval(u_race(u), RCF_SWIM);
    }
    return fval(u_race(u), RCF_FLY);
}

static order *monster_attack(unit * u, const unit * target)
{
    assert(u->region == target->region);
    assert(u->faction != target->faction);
    if (target->faction->flags & FFL_NPC) {
        return NULL;
    }
    if (IS_PAUSED(target->faction)) {
        return NULL;
    }
    if (!cansee(u->faction, u->region, target, 0)) {
        return NULL;
    }
    if (monster_is_waiting(u)) {
        return NULL;
    }

    return create_order(K_ATTACK, u->faction->locale, "%i", target->no);
}

bool join_monsters(unit *u, faction *monsters) {
    if (monsters == NULL) {
        monsters = get_monsters();
        if (monsters == NULL) {
            return false;
        }
    }
    u_setfaction(u, monsters);
    u->status = ST_FIGHT;
    a_removeall(&u->attribs, &at_otherfaction);
    set_group(u, NULL);
    u->flags &= ~(UFL_NOAID | UFL_HERO | UFL_ANON_FACTION);
    u_seteffstealth(u, -1);
    u_freeorders(u);
    return true;
}

void monsters_desert(struct faction *monsters)
{
    region *r;

    assert(monsters!=NULL);
    for (r = regions; r; r = r->next) {
        unit *u;
        
        for (u = r->units; u; u = u->next) {
            if (u->faction == monsters) {
                const struct race * rc = u_race(u);
                if (rc->splitsize < 10) {
                    /* hermit-type monsters eat each other */
                    monster_cannibalism(u);
                }
            } else if (u_race(u)->flags & RCF_DESERT) {
                if (fval(u, UFL_ISNEW))
                    continue;
                if (rng_int() % 100 < 5) {
                    faction* f = u->faction;
                    if (join_monsters(u, monsters)) {
                        ADDMSG(&f->msgs, msg_message("desertion",
                            "unit region", u, r));
                    }
                }
            }
        }
    }
}

int monster_attacks(unit * monster, bool rich_only)
{
    const race *rc_serpent = get_race(RC_SEASERPENT);
    int result = -1;

    if (monster_can_attack(monster)) {
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
                        result = 0;
                        unit_addorder(monster, ord);
                        money += m;
                    }
                }
            }
        }
        return money > 0 ? money : result;
    }
    return result;
}

static order *get_money_for_dragon(region * r, unit * u, int wanted)
{
    int money;
    bool attacks = (attack_chance > 0.0) && armedmen(u, false);

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
    if (attacks && monster_can_attack(u)) {
        int m = monster_attacks(u, true);
        if (m > 0) money += m;
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

    /* Zufaellig eine auswaehlen */

    rr = rng_int() % c;

    /* Durchzaehlen */

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
    /* Zufaellig eine auswaehlen */

    rr = rng_int() % c;

    /* Durchzaehlen */

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
    else if (get_race(RC_TREEMAN) == u_race(u)) {
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
    selist *ql, *rlist = path_regions_in_range(r, range, allowed_dragon);
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

static order *plan_move_to_target(unit * u, const region * target, int moves,
    bool(*allowed) (const region *, const region *))
{
    region *r = u->region;
    region **plan;
    direction_t steps[DRAGON_RANGE];
    int position;

    if (monster_is_waiting(u))
        return NULL;

    plan = path_find(r, target, DRAGON_RANGE, allowed);
    if (plan == NULL)
        return NULL;

    for (position = 0; position != moves && plan[position + 1]; ++position) {
        region *prev = plan[position];
        region *next = plan[position + 1];
        direction_t dir = reldirection(prev, next);
        assert(dir != NODIRECTION && dir != D_SPECIAL);
        steps[position] = dir;
    }

    return make_movement_order(u->faction->locale, steps, position);
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
    const struct locale *lang = u->faction->locale;
    size_t s, len;

    /* can these monsters even study? */
    if (!check_student(u, NULL, SK_PERCEPTION)) {
        return NULL;
    }

    len = arrlen(u->skills);
    /* Monster lernt ein zufaelliges Talent aus allen, in denen es schon
     * Lerntage hat. */
    for (s = 0; s != len; ++s) {
        if (u->skills[s].level > 0) {
            ++c;
        }
    }

    if (c != 0) {
        int n = rng_int() % c + 1;
        
        for (c = 0, s = 0; s != len; ++s) {
            skill* sv = u->skills + s;
            if (sv->level > 0 && rc_can_learn(u->_race, sv->id)) {
                if (++c == n) {
                    return create_order(K_STUDY, lang, "'%s'", skillname(sv->id, lang));
                }
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

static void recruit_dracoids(unit* u, int size)
{
    unit* un;
    const struct locale* lang = u->faction->locale;

    un = create_unit(u->region, u->faction, 0, get_race(RC_DRACOID), 0, NULL, u);
    name_unit(un);
    unit_setstatus(un, ST_FIGHT);
    fset(un, UFL_ISNEW | UFL_MOVED);
    unit_addorder(un, create_order(K_RECRUIT, lang, "%d", size));
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
        move = (rpeasants(r) == 0);   /* when no peasants, move */
        move = move || (rmoney(r) == 0);      /* when no money, move */
    }
    move = move || chance(0.04);         /* 4% chance to change your mind */

    if (rc == rc_wyrm && !move) {
        unit *u2;
        for (u2 = r->units; u2; u2 = u2->next) {
            /* Wyrme sind Einzelgaenger */
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
            long_order = plan_move_to_target(u, tr, 1, allowed_dragon);
        }
        else {
            switch (old_race(rc)) {
            case RC_FIREDRAGON:
                long_order = plan_move_to_target(u, tr, 4, allowed_dragon);
                break;
            case RC_DRAGON:
                long_order = plan_move_to_target(u, tr, 3, allowed_dragon);
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
                    stats_count("monsters.create.dracoid", 1);
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

void monster_cannibalism(unit *u)
{
    unit *u2;

    for (u2 = u->next; u2; u2 = u2->next) {
        if (u2->_race == u->_race) {
            i_merge(&u->items, &u2->items);
            stats_count("monsters.cannibalism", u2->number);
            u2->number = 0;
        }
    }
}

static bool monster_can_learn(const race *rc) {
    return (rc->flags & (RCF_NOLEARN|RCF_AI_LEARN)) == RCF_AI_LEARN;
}

void plan_monsters(faction * f)
{
    region *r;
    
    attack_chance = config_get_flt("rules.monsters.attack_chance", 0.4);

    for (r = regions; r; r = r->next) {
        unit *u;

        for (u = r->units; u; u = u->next) {
            const race *rc = u_race(u);
            attrib *ta;
            order *long_order = NULL;
            bool can_move = true;
            bool guarding;

            /* Ab hier nur noch Befehle fuer NPC-Einheiten. */
            if (u->number <= 0 || (u->faction->flags & FFL_NPC) == 0) {
                continue;
            }
            if (f) {
                if (u->faction != f) {
                    continue;
                }
                f->lastorders = turn;
            }
            else {
                u->faction->lastorders = turn;
            }

            /* Parteitarnung von NPCs ist doof: */
            if (fval(u, UFL_ANON_FACTION)) {
                u->flags &= ~UFL_ANON_FACTION;
            }
            a_removeall(&u->attribs, &at_otherfaction);

            if (skill_enabled(SK_PERCEPTION)) {
                /* Monster bekommen jede Runde ein paar Tage Wahrnehmung dazu */
                produceexp(u, SK_PERCEPTION);
            }

            /* Befehle muessen jede Runde neu gegeben werden: */
            free_orders(&u->orders);

            guarding = is_guard(u);
            /* All monsters want to guard the region: */
            if (!guarding && u->status < ST_FLEE && !monster_is_waiting(u) && r->land) {
                unit_addorder(u, create_order(K_GUARD, u->faction->locale, NULL));
            }

            /* units with a plan to kill get ATTACK orders (even if they don't guard): */
            ta = a_find(u->attribs, &at_hate);
            if (ta && !monster_is_waiting(u)) {
                unit *tu = (unit *)ta->data.v;
                if (tu && tu->region == r && monster_can_attack(u)) {
                    order * ord = monster_attack(u, tu);
                    if (ord) {
                        unit_addorder(u, ord);
                        can_move = false;
                    }
                }
                else if (tu) {
                    bool(*allowed)(const struct region * src, const struct region * r) = allowed_walk;
                    if (canfly(u)) {
                        allowed = allowed_fly;
                    }
                    long_order = plan_move_to_target(u, tu->region, 2, allowed);
                    can_move = false;
                }
                else
                    a_remove(&u->attribs, ta);
            }
            else if (monster_can_attack(u)) {
                if (chance(attack_chance)) {
                    int m = monster_attacks(u, false);
                    if (m >= 0) {
                        can_move = false;
                    }
                }
            }

            /* Einheiten mit Bewegungsplan kriegen ein NACH: */
            if (can_move && long_order == NULL) {
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
                    if (can_move && rc == get_race(RC_SEASERPENT)) {
                        long_order = create_order(K_PIRACY, u->faction->locale, NULL);
                    }
                    else {
                        if (monster_can_learn(rc)) {
                            long_order = monster_learn(u);
                        }
                    }
                }
            }
            if (long_order == NULL && check_student(u, NULL, SK_WEAPONLESS)) {
                /* Einheiten, die Waffenlosen Kampf lernen koennten, lernen es um
                * zu bewachen: */
                if (rc->bonus[SK_WEAPONLESS] != -99) {
                    if (effskill(u, SK_WEAPONLESS, NULL) < 1) {
                        long_order =
                            create_order(K_STUDY, u->faction->locale, "'%s'",
                                skillname(SK_WEAPONLESS, u->faction->locale));
                    }
                }
            }

            if (long_order) {
                unit_addorder(u, long_order);
            }
        }
    }
    pathfinder_cleanup();
}

static int nrand(int handle_start, int sub)
{
    int res = 0;

    do {
        if (rng_int() % 100 < handle_start) {
            res++;
        }
        handle_start -= sub;
    } while (handle_start > 0);

    return res;
}

unit *spawn_seaserpent(region *r, faction *f) {
    unit *u = create_unit(r, f, 1, get_race(RC_SEASERPENT), 0, NULL, NULL);
    stats_count("monsters.create.seaserpent", 1);
    fset(u, UFL_ISNEW | UFL_MOVED);
    equip_unit(u, "seed_seaserpent");
    return u;
}

/** 
 * Drachen und Seeschlangen koennen entstehen
 */
void spawn_dragons(void)
{
    region *r;
    faction *monsters = get_or_create_monsters();
    int minage = config_get_int("monsters.spawn.min_age", 100);
    int spawn_chance = config_get_int("monsters.spawn.chance", 100) * 100;

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
            && rng_int() % spawn_chance < 6)
        {
            message *msg;
            if (chance(0.80)) {
                u = create_unit(r, monsters, nrand(60, 20) + 1, get_race(RC_FIREDRAGON), 0, NULL, NULL);
            }
            else {
                u = create_unit(r, monsters, nrand(30, 20) + 1, get_race(RC_DRAGON), 0, NULL, NULL);
            }
            stats_count("monsters.create.dragon", 1);
            fset(u, UFL_ISNEW | UFL_MOVED);
            equip_unit(u, "seed_dragon");

            log_debug("spawning %d %s in %s.\n", u->number,
                LOC(default_locale,
                    rc_name_s(u_race(u), (u->number == 1) ? NAME_SINGULAR : NAME_PLURAL)), regionname(r, NULL));

            name_unit(u);

            /* add message to the region */
            r_add_warning(r, msg = msg_message("sighting", "region race number", r, u_race(u), u->number));
            msg_release(msg);
        }
    }
}

/** Untote koennen entstehen */
void spawn_undead(void)
{
    region *r;
    faction *monsters = get_monsters();
    int spawn_chance = config_get_int("monsters.spawn.chance", 100) * 100;

    if (spawn_chance <= 0) {
        return;
    }
    for (r = regions; r; r = r->next) {
        int unburied = deathcount(r);

        if (r->attribs) {
            if (curse_active(get_curse(r->attribs, &ct_holyground))) {
                continue;
            }
        }

        if (r->land && unburied > rpeasants(r) / 20
            && rng_int() % spawn_chance < 100) {
            message *msg;
            unit *u;
            /* es ist sinnfrei, wenn irgendwo im Wald 3er-Einheiten Untote entstehen.
             * Lieber sammeln lassen, bis sie mindestens 5% der Bevoelkerung sind, und
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
            stats_count("monsters.create.undead", 1);
            fset(u, UFL_ISNEW | UFL_MOVED);
            if ((rc == get_race(RC_SKELETON) || rc == get_race(RC_ZOMBIE))
                && rng_int() % 10 < 4) {
                equip_unit(u, "rising_undead");
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
          r_add_warning(r, msg);
          msg_release(msg);
        }
        else {
            int i = deathcount(r);
            if (i) {
                /* Groeber verwittern, 3% der Untoten finden die ewige Ruhe */
                deathcounts(r, (int)(-i * 0.03));
            }
        }
    }
}

static void eaten_by_monster(unit * u)
{
    /* adjustment for smaller worlds */
    double multi = newterrain(T_PLAIN)->size / 20000.0;
    const resource_type *rhorse = get_resourcetype(R_HORSE);
    const race *rc = u_race(u);
    int p = rpeasants(u->region);

    if (p > 0) {
        int horse = -1;
        int scare = rc_scare(rc);
        int n = 0;

        if (scare > 0) {
            n = rng_int() % scare * u->number;
        }
        else {
            n = rng_int() % (u->number / 20 + 1);
            horse = 0;
        }

        horse = horse ? i_get(u->items, rhorse->itype) : 0;
        if (horse > 0) {
            i_change(&u->items, rhorse->itype, -horse);
            ADDMSG(&u->region->msgs, msg_message("eathorse", "unit amount", u, horse));
        }

        n = (int)(n * multi);
        if (n > 0) {
            n = lovar(n);

            if (p < n) n = p;
            if (n > 0) {
                if (n > 0) {
                    deathcounts(u->region, n);
                    rsetpeasants(u->region, rpeasants(u->region) - n);
                    ADDMSG(&u->region->msgs, msg_message("eatpeasants", "unit amount", u, n));
                }
            }
        }
    }
}

static void absorbed_by_monster(unit * u)
{
    int n = rng_int() % (u->number / 20 + 1);

    if (n > 0) {
        n = lovar(n);
        if (n > 0) {
            int p = rpeasants(u->region);
            if (p < n) n = p;
            if (n > 0) {
                rsetpeasants(u->region, rpeasants(u->region) - n);
                scale_number(u, u->number + n);
                ADDMSG(&u->region->msgs, msg_message("absorbpeasants",
                    "unit race amount", u, u_race(u), n));
            }
        }
    }
}

static int scareaway(region * r, int anzahl)
{
    int n, p, diff = 0, emigrants[MAXDIRECTIONS];
    direction_t d;

    p = rpeasants(r);
    if (anzahl < 1) anzahl = 1;
    if (anzahl > p) anzahl = p;
    assert(p >= 0 && anzahl >= 0);

    /* Wandern am Ende der Woche (normal) oder wegen Monster. Die
     * Wanderung wird erst am Ende von demographics () ausgefuehrt.
     * emigrants[] ist local, weil r->newpeasants durch die Monster
     * vielleicht schon hochgezaehlt worden ist. */

    for (d = 0; d != MAXDIRECTIONS; d++)
        emigrants[d] = 0;

    for (n = anzahl; n; n--) {
        direction_t dir = (direction_t)(rng_int() % MAXDIRECTIONS);
        region *rc = rconnect(r, dir);

        if (rc && rc->land) {
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
        if (n > 0) {
            int p = rpeasants(u->region);
            if (p < n) n = p;
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
    if (join_monsters(u, NULL)) {
        u_freeorders(u);
        scale_number(u, 1);
        u->hp = unit_max_hp(u) * u->number;
        u_setrace(u, get_race(RC_ZOMBIE));
        u->irace = NULL;
    }
}
