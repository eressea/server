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

#ifdef _MSC_VER
#include <platform.h>
#endif

#include "randenc.h"

#include "chaos.h"
#include "economy.h"
#include "monsters.h"
#include "move.h"
#include "study.h"
#include "volcano.h"

#include "spells/unitcurse.h"
#include "spells/regioncurse.h"

/* attributes includes */
#include "attributes/racename.h"
#include "attributes/reduceproduction.h"

/* kernel includes */
#include "kernel/building.h"
#include "kernel/config.h"
#include "kernel/curse.h"
#include "kernel/equipment.h"
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/plane.h"
#include "kernel/pool.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/ship.h"
#include "kernel/terrain.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"

/* util includes */
#include "util/attrib.h"
#include "util/language.h"
#include "util/lists.h"
#include "util/log.h"
#include "util/rand.h"
#include "util/message.h"
#include "util/rng.h"

/* libc includes */
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <attributes/iceberg.h>
extern struct attrib_type at_unitdissolve;

/* In a->data.ca[1] steht der Prozentsatz mit dem sich die Einheit
 * aufl�st, in a->data.ca[0] kann angegeben werden, wohin die Personen
 * verschwinden. Passiert bereits in der ersten Runde! */
static void dissolve_units(void)
{
    region *r;
    unit *u;
    int n;
    int i;

    for (r = regions; r; r = r->next) {
        for (u = r->units; u; u = u->next) {
            attrib *a = a_find(u->attribs, &at_unitdissolve);
            if (a) {
                message *msg;

                if (u->age == 0 && a->data.ca[1] < 100)
                    continue;

                /* TODO: Durch einzelne Berechnung ersetzen */
                if (a->data.ca[1] == 100) {
                    n = u->number;
                }
                else {
                    n = 0;
                    for (i = 0; i < u->number; i++) {
                        if (rng_int() % 100 < a->data.ca[1])
                            n++;
                    }
                }

                /* wenn keiner verschwindet, auch keine Meldung */
                if (n == 0) {
                    continue;
                }

                scale_number(u, u->number - n);

                switch (a->data.ca[0]) {
                case 1:
                    rsetpeasants(r, rpeasants(r) + n);
                    msg =
                        msg_message("dissolve_units_1", "unit region number race", u, r,
                        n, u_race(u));
                    break;
                case 2:
                    if (r->land && !fval(r, RF_MALLORN)) {
                        rsettrees(r, 2, rtrees(r, 2) + n);
                        msg =
                            msg_message("dissolve_units_2", "unit region number race", u, r,
                            n, u_race(u));
                    }
                    else {
                        msg =
                            msg_message("dissolve_units_3", "unit region number race", u, r,
                            n, u_race(u));
                    }
                    break;
                default:
                    if (u_race(u) == get_race(RC_STONEGOLEM)
                        || u_race(u) == get_race(RC_IRONGOLEM)) {
                        msg =
                            msg_message("dissolve_units_4", "unit region number race", u, r,
                            n, u_race(u));
                    }
                    else {
                        msg =
                            msg_message("dissolve_units_5", "unit region number race", u, r,
                            n, u_race(u));
                    }
                    break;
                }

                add_message(&u->faction->msgs, msg);
                msg_release(msg);
            }
        }
    }

    remove_empty_units();
}

void drown(region * r)
{
    if (fval(r->terrain, SEA_REGION)) {
        unit **up = &r->units;
        while (*up) {
            unit *u = *up;

            if (!(u->ship || u->number == 0 || canswim(u) || canfly(u))) {
                scale_number(u, 0);
                ADDMSG(&u->faction->msgs, msg_message("drown", "unit region", u, r));
            }

            up = &u->next;
        }
        remove_empty_units_in_region(r);
    }
}

static void melt_iceberg(region * r, const terrain_type *t_ocean)
{
    attrib *a;
    unit *u;

    for (u = r->units; u; u = u->next)
        freset(u->faction, FFL_SELECT);
    for (u = r->units; u; u = u->next)
        if (!fval(u->faction, FFL_SELECT)) {
            fset(u->faction, FFL_SELECT);
            ADDMSG(&u->faction->msgs, msg_message("iceberg_melt", "region", r));
        }

    /* driftrichtung l�schen */
    a = a_find(r->attribs, &at_iceberg);
    if (a)
        a_remove(&r->attribs, a);

    /* Geb�ude l�schen */
    while (r->buildings) {
        remove_building(&r->buildings, r->buildings);
    }

    /* in Ozean wandeln */
    terraform_region(r, t_ocean);
}

static void move_iceberg(region * r)
{
    attrib *a;
    direction_t dir;
    region *rc;

    a = a_find(r->attribs, &at_iceberg);
    if (!a) {
        dir = (direction_t)(rng_int() % MAXDIRECTIONS);
        a = a_add(&r->attribs, make_iceberg(dir));
    }
    else {
        if (rng_int() % 100 < 20) {
            dir = (direction_t)(rng_int() % MAXDIRECTIONS);
            a->data.i = dir;
        }
        else {
            dir = (direction_t)a->data.i;
        }
    }

    rc = rconnect(r, dir);

    if (rc && !fval(rc->terrain, ARCTIC_REGION)) {
        if (fval(rc->terrain, SEA_REGION)) {        /* Eisberg treibt */
            ship *sh, *shn;
            unit *u;
            int x, y;

            for (u = r->units; u; u = u->next)
                freset(u->faction, FFL_SELECT);
            for (u = r->units; u; u = u->next)
                if (!fval(u->faction, FFL_SELECT)) {
                    fset(u->faction, FFL_SELECT);
                    ADDMSG(&u->faction->msgs, msg_message("iceberg_drift",
                        "region dir", r, dir));
                }

            x = r->x;
            y = r->y;

            runhash(r);
            runhash(rc);
            r->x = rc->x;
            r->y = rc->y;
            rc->x = x;
            rc->y = y;
            rhash(rc);
            rhash(r);

            /* rc ist der Ozean (Ex-Eisberg), r der Eisberg (Ex-Ozean) */

            /* Schiffe aus dem Zielozean werden in den Eisberg transferiert
             * und nehmen Schaden. */

            for (sh = r->ships; sh; sh = sh->next)
                freset(sh, SF_SELECT);

            for (sh = r->ships; sh; sh = sh->next) {
                /* Meldung an Kapit�n */
                double dmg = config_get_flt("rules.ship.damage.intoiceberg", 0.1);
                damage_ship(sh, dmg);
                fset(sh, SF_SELECT);
            }

            /* Personen, Schiffe und Geb�ude verschieben */
            while (rc->buildings) {
                rc->buildings->region = r;
                translist(&rc->buildings, &r->buildings, rc->buildings);
            }
            while (rc->ships) {
                double dmg = config_get_flt("rules.ship.damage.withiceberg", 0.1);
                fset(rc->ships, SF_SELECT);
                damage_ship(rc->ships, dmg);
                move_ship(rc->ships, rc, r, NULL);
            }
            while (rc->units) {
                building *b = rc->units->building;
                u = rc->units;
                u->building = 0; /* prevent leaving in move_unit */
                move_unit(rc->units, r, NULL);
                u_set_building(u, b); /* undo leave-prevention */
            }

            /* Besch�digte Schiffe k�nnen sinken */

            for (sh = r->ships; sh;) {
                shn = sh->next;
                if (fval(sh, SF_SELECT)) {
                    u = ship_owner(sh);
                    if (sh->damage >= sh->size * DAMAGE_SCALE) {
                        if (u != NULL) {
                            ADDMSG(&u->faction->msgs, msg_message("overrun_by_iceberg_des",
                                "ship", sh));
                        }
                        remove_ship(&sh->region->ships, sh);
                    }
                    else if (u != NULL) {
                        ADDMSG(&u->faction->msgs, msg_message("overrun_by_iceberg",
                            "ship", sh));
                    }
                }
                sh = shn;
            }

        }
        else if (rng_int() % 100 < 20) {  /* Eisberg bleibt als Gletscher liegen */
            unit *u;

            rsetterrain(r, T_GLACIER);
            a_remove(&r->attribs, a);

            for (u = r->units; u; u = u->next)
                freset(u->faction, FFL_SELECT);
            for (u = r->units; u; u = u->next)
                if (!fval(u->faction, FFL_SELECT)) {
                    fset(u->faction, FFL_SELECT);
                    ADDMSG(&u->faction->msgs, msg_message("iceberg_land", "region", r));
                }
        }
    }
}

static void move_icebergs(void)
{
    region *r;
    static int terrain_cache;
    static const terrain_type *t_iceberg, *t_ocean;

    if (terrain_changed(&terrain_cache)) {
        t_iceberg = newterrain(T_ICEBERG);
        t_ocean = newterrain(T_OCEAN);
    }
    for (r = regions; r; r = r->next) {
        if (r->terrain == t_iceberg && !fval(r, RF_SELECT)) {
            int select = rng_int() % 10;
            if (select < 4) {
                /* 4% chance */
                fset(r, RF_SELECT);
                melt_iceberg(r, t_ocean);
            }
            else if (select < 64) {
                /* 60% chance */
                fset(r, RF_SELECT);
                move_iceberg(r);
            }
        }
    }
}

static void create_icebergs(void)
{
    region *r;
    const struct terrain_type *t_iceberg, *t_sleep;

    t_iceberg = get_terrain("iceberg");
    t_sleep = get_terrain("iceberg_sleep");
    assert(t_iceberg && t_sleep);

    for (r = regions; r; r = r->next) {
        if (r->terrain == t_sleep && chance(0.05)) {
            bool has_ocean_neighbour = false;
            direction_t dir;
            unit *u;

            freset(r, RF_SELECT);
            for (dir = 0; dir < MAXDIRECTIONS; dir++) {
                region *rc = rconnect(r, dir);
                if (rc && fval(rc->terrain, SEA_REGION)) {
                    has_ocean_neighbour = true;
                    break;
                }
            }
            if (!has_ocean_neighbour)
                continue;

            r->terrain = t_iceberg;

            fset(r, RF_SELECT);
            move_iceberg(r);

            for (u = r->units; u; u = u->next) {
                freset(u->faction, FFL_SELECT);
            }
            for (u = r->units; u; u = u->next) {
                if (!fval(u->faction, FFL_SELECT)) {
                    fset(u->faction, FFL_SELECT);
                    ADDMSG(&u->faction->msgs, msg_message("iceberg_create", "region", r));
                }
            }
        }
    }
}

static void godcurse(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        if (is_cursed(r->attribs, &ct_godcursezone)) {
            unit *u;
            for (u = r->units; u; u = u->next) {
                skill *sv = u->skills;
                while (sv != u->skills + u->skill_size) {
                    int weeks = 1 + rng_int() % 3;
                    reduce_skill(u, sv, weeks);
                    ++sv;
                }
            }
            if (fval(r->terrain, SEA_REGION)) {
                ship *sh;
                for (sh = r->ships; sh;) {
                    ship *shn = sh->next;
                    double dmg = config_get_flt("rules.ship.damage.godcurse", 0.1);
                    damage_ship(sh, dmg);
                    if (sh->damage >= sh->size * DAMAGE_SCALE) {
                        unit *uo = ship_owner(sh);
                        if (uo) {
                            ADDMSG(&uo->faction->msgs,
                                msg_message("godcurse_destroy_ship", "ship", sh));
                        }
                        remove_ship(&sh->region->ships, sh);
                    }
                    sh = shn;
                }
            }
        }
    }

}

/** handles the "orcish" curse that makes units grow like old orks
 * TODO: This would probably be better handled in an age-function for the curse,
 * but it's now being called by randomevents()
 */
static void orc_growth(void)
{
    region *r;
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (u->attribs && !has_skill(u, SK_MAGIC) && !has_skill(u, SK_ALCHEMY)
                && !fval(u, UFL_HERO)) {
                curse *c = get_curse(u->attribs, &ct_orcish);
                if (c) {
                    int n;
                    int increase = 0;
                    int num = get_cursedmen(u, c);
                    double prob = curse_geteffect(c);
                    const item_type * it_chastity = it_find("ao_chastity");

                    if (it_chastity) {
                        num -= i_get(u->items, it_chastity);
                    }
                    for (n = num; n > 0; n--) {
                        if (chance(prob)) {
                            ++increase;
                        }
                    }
                    if (increase) {
                        unit *u2 = create_unit(r, u->faction, increase, u_race(u), 0, NULL, u);
                        transfermen(u2, u, u2->number);

                        ADDMSG(&u->faction->msgs, msg_message("orcgrowth",
                            "unit amount race", u, increase, u_race(u)));
                    }
                }
            }
        }
    }
}

/** Talente von D�monen verschieben sich.
 */
static void demon_skillchanges(void)
{
    region *r;
    static const race *rc_demon;
    static int rc_cache;

    if (rc_changed(&rc_cache)) {
        rc_demon = get_race(RC_DAEMON);
    }
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (u_race(u) == rc_demon) {
                demon_skillchange(u);
            }
        }
    }
}

/** Eisberge entstehen und bewegen sich.
 * Einheiten die im Wasser landen, ertrinken.
 */
static void icebergs(void)
{
    create_icebergs();
    move_icebergs();
}

#define HERBROTCHANCE 5         /* Verrottchance f�r Kr�uter (ifdef HERBS_ROT) */

static void rotting_herbs(void)
{
    region *r;
    int rule_rot = config_get_int("rules.economy.herbrot", HERBROTCHANCE);
    if (rule_rot == 0) return;

    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            const struct item_type *it_bag = it_find("magicherbbag");
            item **itmp = &u->items;
            int rot_chance = rule_rot;

            if (it_bag && *i_find(itmp, it_bag)) {
                rot_chance = (rot_chance * 2) / 5;
            }
            while (*itmp) {
                item *itm = *itmp;
                int n = itm->number;
                double k = n * rot_chance / 100.0;
                if (fval(itm->type, ITF_HERB)) {
                    int delta = (int)normalvariate(k, k / 4);
                    if (n < delta) delta = n;
                    if (!i_change(itmp, itm->type, -delta)) {
                        continue;
                    }
                }
                itmp = &itm->next;
            }
        }
    }
}

void randomevents(void)
{
    region *r;
    faction *monsters = get_monsters();

    if (config_get_int("modules.iceberg", 0)) {
        icebergs();
    }
    for (r = regions; r; r = r->next) {
        drown(r);
    }
    godcurse();
    orc_growth();
    demon_skillchanges();
    if (volcano_module()) {
        volcano_update();
    }
    /* Monumente zerfallen, Schiffe verfaulen */

    for (r = regions; r; r = r->next) {
        building **blist = &r->buildings;
        while (*blist) {
            building *b = *blist;
            if (fval(b->type, BTF_DECAY) && !building_owner(b)) {
                int delta = (b->size * 20) / 100;
                if (delta < 1) delta = 1;
                b->size -= delta;
                if (b->size == 0) {
                    remove_building(blist, r->buildings);
                }
            }
            if (*blist == b)
                blist = &b->next;
        }
    }

    /* monster-einheiten desertieren */
    if (monsters) {
        monsters_desert(monsters);
    }

    chaos_update();
    rotting_herbs();

    dissolve_units();
}

void plagues(region * r)
{
    int peasants;
    int i;
    int dead = 0;

    peasants = rpeasants(r);
    dead = (int)(0.5 + PLAGUE_VICTIMS * peasants);
    for (i = dead; i != 0; i--) {
        if (rng_double() < PLAGUE_HEALCHANCE && rmoney(r) >= PLAGUE_HEALCOST) {
            rsetmoney(r, rmoney(r) - PLAGUE_HEALCOST);
            --dead;
        }
    }

    if (dead > 0) {
        message *msg = add_message(&r->msgs, msg_message("pest", "dead", dead));
        msg_release(msg);
        deathcounts(r, dead);
        rsetpeasants(r, peasants - dead);
    }
}
