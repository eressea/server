#include <kernel/config.h>
#include "volcano.h"

#include "alchemy.h"

/* attributes includes */
#include "attributes/reduceproduction.h"

/* kernel includes */
#include "kernel/attrib.h"
#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/terrain.h"
#include "kernel/unit.h"

/* util includes */
#include "util/log.h"
#include "util/rand.h"
#include "util/message.h"
#include "util/rng.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>

static int nb_armor(const unit * u, int index)
{
    const item *itm;
    int av = 0;
    int s = 0, a = 0;

    if (!(u_race(u)->battle_flags & BF_EQUIPMENT))
        return 0;

    /* Normale Ruestung */

    for (itm = u->items; itm; itm = itm->next) {
        const armor_type *atype = itm->type->rtype->atype;
        if (atype != NULL) {
            int *schutz = &a;
            if (atype->flags & ATF_SHIELD)
                schutz = &s;
            if (*schutz <= index) {
                *schutz += itm->number;
                if (*schutz > index) {
                    av += atype->prot;
                }
            }
        }
    }
    return av;
}

static bool resurrect_unit(unit *u) {
    if (oldpotiontype[P_HEAL]) {
        bool heiltrank = false;
        if (get_effect(u, oldpotiontype[P_HEAL]) > 0) {
            change_effect(u, oldpotiontype[P_HEAL], -1);
            heiltrank = true;
        }
        else if (i_get(u->items, oldpotiontype[P_HEAL]) > 0) {
            i_change(&u->items, oldpotiontype[P_HEAL], -1);
            change_effect(u, oldpotiontype[P_HEAL], 3);
            heiltrank = true;
        }
        if (heiltrank && (rng_int() % 2)) {
            return true;
        }
    }
    return false;
}

int volcano_damage(unit* u, const char* dice)
{
    int hp = u->hp / u->number;
    int remain = u->hp % u->number;
    int ac = 0, i, dead = 0, total = 0;
    int healings = 0;
    const struct race* rc_cat = get_race(RC_CAT);
    int protect = inside_building(u) ? (building_protection(u->building) + 1) : 0;

    /* does this unit have any healing potions or effects? */
    if (oldpotiontype[P_HEAL]) {
        healings = get_effect(u, oldpotiontype[P_HEAL]);
        healings += i_get(u->items, oldpotiontype[P_HEAL]) * 4;
    }

    for (i = 0; i != u->number; ++i) {
        int h = hp + ((i < remain) ? 1 : 0);
        int damage = dice_rand(dice) - protect;
        if (damage > 0) {
            if (i == 0 || ac > 0) {
                ac = nb_armor(u, i);
                damage -= ac;
            }
            if (damage > 0) {
                if (damage >= h) {
                    if (rc_cat && u_race(u) == rc_cat && ((rng_int() % 7) == 0)) {
                        /* cats have nine lives */
                        continue;
                    }
                    else if (healings > 0) {
                        --healings;
                        if (resurrect_unit(u)) {
                            /* take no damage at all */
                            continue;
                        }
                    }
                    ++dead;
                    damage = h;
                }
                total += damage;
            }
        }
    }
    total = u->hp - total;
    scale_number(u, u->number - dead);
    u->hp = total;
    return dead;
}

static region *rrandneighbour(region * r)
{
    direction_t i;
    region *rc = NULL;
    int rr, c = 0;

    /* Nachsehen, wieviele Regionen in Frage kommen */

    for (i = 0; i != MAXDIRECTIONS; i++) {
        c++;
    }
    /* Zufaellig eine auswaehlen */

    rr = rng_int() % c;

    /* Durchzaehlen */

    c = -1;
    for (i = 0; i != MAXDIRECTIONS; i++) {
        rc = rconnect(r, i);
        c++;
        if (c == rr)
            break;
    }
    assert(i != MAXDIRECTIONS);
    return rc;
}

static void
volcano_destruction(region * volcano, region * r, const char *damage)
{
    attrib *a;
    unit **up;
    int percent = 25, time = 6 + rng_int() % 12;

    rsettrees(r, 2, 0);
    rsettrees(r, 1, 0);
    rsettrees(r, 0, 0);

    a = a_find(r->attribs, &at_reduceproduction);
    if (!a) {
        a = a_add(&r->attribs, make_reduceproduction(percent, time));
    }
    else {
        /* Produktion vierteln ... */
        a->data.sa[0] = (short)percent;
        /* Fuer 6-17 Runden */
        a->data.sa[1] = (short)(a->data.sa[1] + time);
    }

    /* Personen bekommen 4W10 Punkte Schaden. */

    for (up = &r->units; *up;) {
        unit *u = *up;
        if (u->number) {
            int dead = volcano_damage(u, damage);
            /* TODO create undead */
            if (dead) {
                ADDMSG(&u->faction->msgs, msg_message("volcano_dead",
                    "unit region dead", u, volcano, dead));
            }
            if (!fval(u->faction, FFL_SELECT)) {
                fset(u->faction, FFL_SELECT);
                ADDMSG(&u->faction->msgs, msg_message("volcanooutbreaknn",
                    "region", volcano));
            }
        }
        if (u == *up) {
            up = &u->next;
        }
    }

    if (r != volcano) {
        ADDMSG(&r->msgs, msg_message("volcanooutbreak",
            "regionv regionn", volcano, r));
    }
    remove_empty_units_in_region(r);
}

void volcano_outbreak(region * r, region *rn)
{
    unit *u;
    faction *f;

    for (f = NULL, u = r->units; u; u = u->next) {
        if (f != u->faction) {
            f = u->faction;
            freset(f, FFL_SELECT);
        }
    }

    volcano_destruction(r, r, "4d10");
    if (rn) {
        volcano_destruction(r, rn, "3d10");
    }
}

static bool stop_smoke_chance(void) {
    static int cache, percent = 0;
    if (config_changed(&cache)) {
        percent = config_get_int("volcano.stop.percent", 12);
    }
    return percent != 0 && (rng_int() % 100) < percent;
}

static bool outbreak_chance(void) {
    static int cache, percent = 0;
    if (config_changed(&cache)) {
        percent = config_get_int("volcano.outbreak.percent", 8);
    }
    return percent != 0 && (rng_int() % 100) < percent;
}

void volcano_update(void)
{
    region *r;
    const struct terrain_type *t_active, *t_volcano;

    t_volcano = get_terrain("volcano");
    t_active = get_terrain("activevolcano");
    /* Vulkane qualmen, brechen aus ... */
    for (r = regions; r; r = r->next) {
        if (r->terrain == t_active) {
            if (a_find(r->attribs, &at_reduceproduction)) {
                ADDMSG(&r->msgs, msg_message("volcanostopsmoke", "region", r));
                r->terrain = t_volcano;
            }
            else {
                if (stop_smoke_chance()) {
                    ADDMSG(&r->msgs, msg_message("volcanostopsmoke", "region", r));
                    r->terrain = t_volcano;
                }
                else if (r->uid == 1246051340 || outbreak_chance()) {
                    /* HACK: a fixed E4-only region-uid in Code.
                     * FIXME: In E4 gibt es eine Ebene #1246051340, die Smalland heisst.
                     * da das kein aktiver Vulkan ist, ist dieser Test da nicht idiotisch?
                     * das sollte bestimmt rn betreffen? */
                    region *rn;
                    rn = rrandneighbour(r);
                    volcano_outbreak(r, rn);
                    r->terrain = t_volcano;
                }
            }
        }
        else if (r->terrain == t_volcano) {
            int volcano_chance = config_get_int("volcano.active.percent", 4);
            if (rng_int() % 100 < volcano_chance) {
                ADDMSG(&r->msgs, msg_message("volcanostartsmoke", "region", r));
                r->terrain = t_active;
            }
        }
    }
}

bool volcano_module(void)
{
    static int cache;
    static bool active;
    if (config_changed(&cache)) {
        active = config_get_int("modules.volcano", 1) != 0;
    }
    return active;
}
