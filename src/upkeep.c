#include <platform.h>
#include "upkeep.h"

#include <kernel/ally.h>
#include <kernel/faction.h>
#include <kernel/config.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <util/rand.h>

#include "alchemy.h"
#include "economy.h"
#include "monsters.h"
#include "donations.h"

#include <assert.h>

int lifestyle(const unit * u)
{
    int need;
    plane *pl;

    if (is_monsters(u->faction))
        return 0;

    need = maintenance_cost(u);

    pl = rplane(u->region);
    if (pl && fval(pl, PFL_NOFEED))
        return 0;

    return need;
}

static bool help_money(const unit * u)
{
    return !(u_race(u)->ec_flags & ECF_KEEP_ITEM);
}

static void help_feed(unit * donor, unit * u, int *need_p)
{
    int need = *need_p;
    int give = get_money(donor) - lifestyle(donor);
    give = MIN(need, give);

    if (give > 0) {
        change_money(donor, -give);
        change_money(u, give);
        need -= give;
        add_donation(donor->faction, u->faction, give, donor->region);
    }
    *need_p = need;
}

static const char *hunger_damage(const race *rc) {
    const char * damage = rc->parameters ? get_param(rc->parameters, "hunger.damage") : NULL;
    if (!damage) {
        damage = config_get("hunger.damage");
    }
    if (!damage) {
        damage = "1d12+12";
    }
    return damage;
}

static bool hunger(int number, unit * u)
{
    region *r = u->region;
    int dead = 0, hpsub = 0;
    int hp = u->hp / u->number;
    const char *damage = 0;

    damage = hunger_damage(u_race(u));

    while (number--) {
        int dam = dice_rand(damage);
        if (dam >= hp) {
            ++dead;
        }
        else {
            hpsub += dam;
        }
    }

    if (dead) {
        /* Gestorbene aus der Einheit nehmen,
        * Sie bekommen keine Beerdingung. */
        ADDMSG(&u->faction->msgs, msg_message("starvation",
            "unit region dead live", u, r, dead, u->number - dead));

        scale_number(u, u->number - dead);
        deathcounts(r, dead);
    }
    if (hpsub > 0) {
        /* Jetzt die Schäden der nicht gestorbenen abziehen. */
        u->hp -= hpsub;
        /* Meldung nur, wenn noch keine für Tote generiert. */
        if (dead == 0) {
            /* Durch unzureichende Ernährung wird %s geschwächt */
            ADDMSG(&u->faction->msgs, msg_message("malnourish", "unit region", u, r));
        }
    }
    return (dead || hpsub);
}

void get_food(region * r)
{
    plane *pl = rplane(r);
    unit *u;
    int peasantfood = rpeasants(r) * 10;
    int food_rules = config_get_int("rules.food.flags", 0);
    static const race *rc_demon;
    static int rc_cache;
    
    if (rc_changed(&rc_cache)) {
        rc_demon = get_race(RC_DAEMON);
    }
    if (food_rules & FOOD_IS_FREE) {
        return;
    }
    /* 1. Versorgung von eigenen Einheiten. Das vorhandene Silber
    * wird zunächst so auf die Einheiten aufgeteilt, dass idealerweise
    * jede Einheit genug Silber für ihren Unterhalt hat. */

    for (u = r->units; u; u = u->next) {
        int need = lifestyle(u);

        /* Erstmal zurücksetzen */
        freset(u, UFL_HUNGER);

        if (u->ship && (u->ship->flags & SF_FISHING)) {
            unit *v;
            int c = 2;
            for (v = u; c > 0 && v; v = v->next) {
                if (v->ship == u->ship) {
                    int get = 0;
                    if (v->number <= c) {
                        get = lifestyle(v);
                    }
                    else {
                        get = lifestyle(v) * c / v->number;
                    }
                    if (get) {
                        change_money(v, get);
                    }
                }
                c -= v->number;
            }
            u->ship->flags -= SF_FISHING;
        }

        if (food_rules & FOOD_FROM_PEASANTS) {
            struct faction *owner = region_get_owner(r);
            /* if the region is owned, and the owner is nice, then we'll get
            * food from the peasants - should not be used with WORK */
            if (owner != NULL && (get_alliance(owner, u->faction) & HELP_MONEY)) {
                int rm = rmoney(r);
                int use = MIN(rm, need);
                rsetmoney(r, rm - use);
                need -= use;
            }
        }

        need -= get_money(u);
        if (need > 0) {
            unit *v;

            for (v = r->units; need && v; v = v->next) {
                if (v->faction == u->faction && help_money(v)) {
                    int give = get_money(v) - lifestyle(v);
                    give = MIN(need, give);
                    if (give > 0) {
                        change_money(v, -give);
                        change_money(u, give);
                        need -= give;
                    }
                }
            }
        }
    }

    /* 2. Versorgung durch Fremde. Das Silber alliierter Einheiten wird
    * entsprechend verteilt. */
    for (u = r->units; u; u = u->next) {
        int need = lifestyle(u);
        faction *f = u->faction;

        need -= MAX(0, get_money(u));

        if (need > 0) {
            unit *v;

            if (food_rules & FOOD_FROM_OWNER) {
                /* the owner of the region is the first faction to help out when you're hungry */
                faction *owner = region_get_owner(r);
                if (owner && owner != u->faction) {
                    for (v = r->units; v; v = v->next) {
                        if (v->faction == owner && alliedunit(v, f, HELP_MONEY)
                            && help_money(v)) {
                            help_feed(v, u, &need);
                            break;
                        }
                    }
                }
            }
            for (v = r->units; need && v; v = v->next) {
                if (v->faction != f && alliedunit(v, f, HELP_MONEY)
                    && help_money(v)) {
                    help_feed(v, u, &need);
                }
            }

            /* Die Einheit hat nicht genug Geld zusammengekratzt und
            * nimmt Schaden: */
            if (need > 0) {
                int lspp = lifestyle(u) / u->number;
                if (lspp > 0) {
                    int number = (need + lspp - 1) / lspp;
                    if (hunger(number, u))
                        fset(u, UFL_HUNGER);
                }
            }
        }
    }

    /* 3. bestimmen, wie viele Bauern gefressen werden.
    * bei fehlenden Bauern den Dämon hungern lassen
    */
    for (u = r->units; u; u = u->next) {
        if (u_race(u) == rc_demon) {
            int hungry = u->number;

            /* use peasantblood before eating the peasants themselves */
            const struct potion_type *pt_blood = 0;
            const resource_type *rt_blood = rt_find("peasantblood");
            if (rt_blood) {
                pt_blood = rt_blood->ptype;
            }
            if (pt_blood) {
                /* always start with the unit itself, then the first known unit that may have some blood */
                unit *donor = u;
                while (donor != NULL && hungry > 0) {
                    int blut = get_effect(donor, pt_blood);
                    blut = MIN(blut, hungry);
                    if (blut) {
                        change_effect(donor, pt_blood, -blut);
                        hungry -= blut;
                    }
                    if (donor == u)
                        donor = r->units;
                    while (donor != NULL) {
                        if (u_race(donor) == rc_demon && donor != u) {
                            if (get_effect(donor, pt_blood)) {
                                /* if he's in our faction, drain him: */
                                if (donor->faction == u->faction)
                                    break;
                            }
                        }
                        donor = donor->next;
                    }
                }
            }
            /* remaining demons feed on peasants */
            if (pl == NULL || !fval(pl, PFL_NOFEED)) {
                if (peasantfood >= hungry) {
                    peasantfood -= hungry;
                    hungry = 0;
                }
                else {
                    hungry -= peasantfood;
                    peasantfood = 0;
                }
                if (hungry > 0) {
                    bool demon_hunger = config_get_int("hunger.demons.peasant_tolerance", 0) == 0;
                    if (demon_hunger) {
                        /* demons who don't feed are hungry */
                        if (hunger(hungry, u))
                            fset(u, UFL_HUNGER);
                    }
                    else {
                        /* no damage, but set the hungry-flag */
                        fset(u, UFL_HUNGER);
                    }
                }
            }
        }
    }
    rsetpeasants(r, peasantfood / 10);

    /* 3. Von den überlebenden das Geld abziehen: */
    for (u = r->units; u; u = u->next) {
        int need = MIN(get_money(u), lifestyle(u));
        change_money(u, -need);
    }
}
