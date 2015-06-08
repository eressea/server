#include <platform.h>
#include "upkeep.h"

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
#include "monster.h"

#include <assert.h>

int lifestyle(const unit * u)
{
    int need;
    plane *pl;
    static int gamecookie = -1;
    if (gamecookie != global.cookie) {
        gamecookie = global.cookie;
    }

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
    give = _min(need, give);

    if (give > 0) {
        change_money(donor, -give);
        change_money(u, give);
        need -= give;
        add_spende(donor->faction, u->faction, give, donor->region);
    }
    *need_p = need;
}

static bool hunger(int number, unit * u)
{
    region *r = u->region;
    int dead = 0, hpsub = 0;
    int hp = u->hp / u->number;
    static const char *damage = 0;
    static const char *rcdamage = 0;
    static const race *rc = 0;

    if (!damage) {
        damage = get_param(global.parameters, "hunger.damage");
        if (damage == NULL)
            damage = "1d12+12";
    }
    if (rc != u_race(u)) {
        rcdamage = get_param(u_race(u)->parameters, "hunger.damage");
        rc = u_race(u);
    }

    while (number--) {
        int dam = dice_rand(rcdamage ? rcdamage : damage);
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
    static int food_rules = -1;
    static int gamecookie = -1;

    if (food_rules < 0 || gamecookie != global.cookie) {
        gamecookie = global.cookie;
        food_rules = get_param_int(global.parameters, "rules.economy.food", 0);
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
                int use = _min(rm, need);
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
                    give = _min(need, give);
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

        need -= _max(0, get_money(u));

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
        if (u_race(u) == get_race(RC_DAEMON)) {
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
                    blut = _min(blut, hungry);
                    if (blut) {
                        change_effect(donor, pt_blood, -blut);
                        hungry -= blut;
                    }
                    if (donor == u)
                        donor = r->units;
                    while (donor != NULL) {
                        if (u_race(donor) == get_race(RC_DAEMON) && donor != u) {
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
                    static int demon_hunger = -1;
                    if (demon_hunger < 0) {
                        demon_hunger = get_param_int(global.parameters, "hunger.demons", 0);
                    }
                    if (demon_hunger == 0) {
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
        int need = _min(get_money(u), lifestyle(u));
        change_money(u, -need);
    }
}
