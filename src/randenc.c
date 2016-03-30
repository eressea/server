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
#include "randenc.h"

#include "economy.h"
#include "monster.h"
#include "move.h"
#include "alchemy.h"
#include "chaos.h"
#include "study.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/curse.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/reduceproduction.h>

/* util includes */
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/message.h>
#include <util/rng.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <attributes/iceberg.h>
extern struct attrib_type at_unitdissolve;
extern struct attrib_type at_orcification;

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

static bool improve_all(faction * f, skill_t sk, int by_weeks)
{
    unit *u;
    bool result = false;
    for (u = f->units; u; u = u->nextF) {
        if (has_skill(u, sk)) {
            learn_skill(u, sk, by_weeks * STUDYDAYS);
            result = true;
        }
    }
    return result;
}

void find_manual(region * r, unit * u)
{
    char zLocation[32];
    char zBook[32];
    skill_t skill = NOSKILL;
    message *msg;

    switch (rng_int() % 36) {
    case 0:
        skill = SK_MAGIC;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        skill = SK_WEAPONSMITH;
        break;
    case 5:
    case 6:
        skill = SK_TACTICS;
        break;
    case 7:
    case 8:
    case 9:
    case 10:
        skill = SK_SHIPBUILDING;
        break;
    case 11:
    case 12:
    case 13:
    case 14:
        skill = SK_SAILING;
        break;
    case 15:
    case 16:
    case 17:
        skill = SK_HERBALISM;
        break;
    case 18:
    case 19:
        skill = SK_ALCHEMY;
        break;
    case 20:
    case 21:
    case 22:
    case 23:
        skill = SK_BUILDING;
        break;
    case 24:
    case 25:
    case 26:
    case 27:
        skill = SK_ARMORER;
        break;
    case 28:
    case 29:
    case 30:
    case 31:
        skill = SK_MINING;
        break;
    case 32:
    case 33:
    case 34:
    case 35:
        skill = SK_ENTERTAINMENT;
        break;
    }

    slprintf(zLocation, sizeof(zLocation), "manual_location_%d",
        (int)(rng_int() % 4));
    slprintf(zBook, sizeof(zLocation), "manual_title_%s", skillnames[skill]);

    msg = msg_message("find_manual", "unit location book", u, zLocation, zBook);
    if (msg) {
        r_addmessage(r, u->faction, msg);
        msg_release(msg);
    }

    if (!improve_all(u->faction, skill, 3)) {
        learn_skill(u, skill, 9 * STUDYDAYS);
    }
}

static void get_villagers(region * r, unit * u)
{
    unit *newunit;
    message *msg = msg_message("encounter_villagers", "unit", u);
    const char *name = LOC(u->faction->locale, "villagers");

    r_addmessage(r, u->faction, msg);
    msg_release(msg);

    newunit =
        create_unit(r, u->faction, rng_int() % 20 + 3, u->faction->race, 0, name,
        u);
    leave(newunit, true);
    fset(newunit, UFL_ISNEW | UFL_MOVED);
    equip_unit(newunit, get_equipment("random_villagers"));
}

static void get_allies(region * r, unit * u)
{
    unit *newunit = NULL;
    const char *name;
    const char *equip;
    int number;
    message *msg;

    assert(u->number);

    switch (rterrain(r)) {
    case T_PLAIN:
        if (!r_isforest(r)) {
            if (get_money(u) / u->number < 100 + rng_int() % 200)
                return;
            name = "random_plain_men";
            equip = "random_plain";
            number = rng_int() % 8 + 2;
            break;
        }
        else {
            if (effskill(u, SK_LONGBOW, r) < 3
                && effskill(u, SK_HERBALISM, r) < 2
                && effskill(u, SK_MAGIC, r) < 2) {
                return;
            }
            name = "random_forest_men";
            equip = "random_forest";
            number = rng_int() % 6 + 2;
        }
        break;

    case T_SWAMP:
        if (effskill(u, SK_MELEE, r) <= 1) {
            return;
        }
        name = "random_swamp_men";
        equip = "random_swamp";
        number = rng_int() % 6 + 2;
        break;

    case T_DESERT:
        if (effskill(u, SK_RIDING, r) <= 2) {
            return;
        }
        name = "random_desert_men";
        equip = "random_desert";
        number = rng_int() % 12 + 2;
        break;

    case T_HIGHLAND:
        if (effskill(u, SK_MELEE, r) <= 1) {
            return;
        }
        name = "random_highland_men";
        equip = "random_highland";
        number = rng_int() % 8 + 2;
        break;

    case T_MOUNTAIN:
        if (effskill(u, SK_MELEE, r) <= 1 || effskill(u, SK_TRADE, r) <= 2) {
            return;
        }
        name = "random_mountain_men";
        equip = "random_mountain";
        number = rng_int() % 6 + 2;
        break;

    case T_GLACIER:
        if (effskill(u, SK_MELEE, r) <= 1 || effskill(u, SK_TRADE, r) <= 1) {
            return;
        }
        name = "random_glacier_men";
        equip = "random_glacier";
        number = rng_int() % 4 + 2;
        break;

    default:
        return;
    }

    newunit =
        create_unit(r, u->faction, number, u->faction->race, 0,
        LOC(u->faction->locale, name), u);
    equip_unit(newunit, get_equipment(equip));

    u_setfaction(newunit, u->faction);
    set_racename(&newunit->attribs, get_racename(u->attribs));
    if (u_race(u)->flags & RCF_SHAPESHIFT) {
        newunit->irace = u->irace;
    }
    if (fval(u, UFL_ANON_FACTION))
        fset(newunit, UFL_ANON_FACTION);
    fset(newunit, UFL_ISNEW);

    msg = msg_message("encounter_allies", "unit name", u, name);
    r_addmessage(r, u->faction, msg);
    msg_release(msg);
}

static void encounter(region * r, unit * u)
{
    if (!fval(r, RF_ENCOUNTER))
        return;
    freset(r, RF_ENCOUNTER);
    if (rng_int() % 100 >= ENCCHANCE)
        return;
    switch (rng_int() % 3) {
    case 0:
        find_manual(r, u);
        break;
    case 1:
        get_villagers(r, u);
        break;
    case 2:
        get_allies(r, u);
        break;
    }
}

void encounters(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        if (fval(r->terrain, LAND_REGION) && fval(r, RF_ENCOUNTER)) {
            int c = 0;
            unit *u;
            for (u = r->units; u; u = u->next) {
                c += u->number;
            }

            if (c > 0) {
                int i = 0;
                int n = rng_int() % c;

                for (u = r->units; u; u = u->next) {
                    if (i + u->number > n)
                        break;
                    i += u->number;
                }
                assert(u && u->number);
                encounter(r, u);
            }
        }
    }
}

static int nb_armor(const unit * u, int index)
{
    const item *itm;
    int av = 0;
    int s = 0, a = 0;

    if (!(u_race(u)->battle_flags & BF_EQUIPMENT))
        return 0;

    /* Normale R�stung */

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

static int
damage_unit(unit * u, const char *dam, bool physical, bool magic)
{
    int *hp, hpstack[20];
    int h;
    int i, dead = 0, hp_rem = 0, heiltrank;
    double magres = magic_resistance(u);

    assert(u->number);
    if (fval(u_race(u), RCF_ILLUSIONARY) || u_race(u) == get_race(RC_SPELL)) {
        return 0;
    }

    assert(u->number <= u->hp);
    h = u->hp / u->number;
    /* HP verteilen */
    if (u->number < 20) {
        hp = hpstack;
    }
    else {
        hp = malloc(u->number * sizeof(int));
    }
    for (i = 0; i < u->number; i++)
        hp[i] = h;
    h = u->hp - (u->number * h);
    for (i = 0; i < h; i++)
        hp[i]++;

    /* Schaden */
    for (i = 0; i < u->number; i++) {
        int damage = dice_rand(dam);
        if (magic)
            damage = (int)(damage * (1.0 - magres));
        if (physical)
            damage -= nb_armor(u, i);
        hp[i] -= damage;
    }

    /* Auswirkungen */
    for (i = 0; i < u->number; i++) {
        if (hp[i] <= 0) {
            heiltrank = 0;

            /* Sieben Leben */
            if (old_race(u_race(u)) == RC_CAT && (chance(1.0 / 7))) {
                hp[i] = u->hp / u->number;
                hp_rem += hp[i];
                continue;
            }

            /* Heiltrank */
            if (oldpotiontype[P_HEAL]) {
                if (get_effect(u, oldpotiontype[P_HEAL]) > 0) {
                    change_effect(u, oldpotiontype[P_HEAL], -1);
                    heiltrank = 1;
                }
                else if (i_get(u->items, oldpotiontype[P_HEAL]->itype) > 0) {
                    i_change(&u->items, oldpotiontype[P_HEAL]->itype, -1);
                    change_effect(u, oldpotiontype[P_HEAL], 3);
                    heiltrank = 1;
                }
                if (heiltrank && (chance(0.50))) {
                    hp[i] = u->hp / u->number;
                    hp_rem += hp[i];
                    continue;
                }
            }
            dead++;
        }
        else {
            hp_rem += hp[i];
        }
    }

    scale_number(u, u->number - dead);
    u->hp = hp_rem;

    if (hp != hpstack) {
        free(hp);
    }

    return dead;
}

void drown(region * r)
{
    if (fval(r->terrain, SEA_REGION)) {
        unit **up = &r->units;
        while (*up) {
            unit *u = *up;

            if (!(u->ship || u_race(u) == get_race(RC_SPELL) || u->number == 0 || canswim(u) || canfly(u))) {
                scale_number(u, 0);
                ADDMSG(&u->faction->msgs, msg_message("drown", "unit region", u, r));
            }

            up = &u->next;
        }
        remove_empty_units_in_region(r);
    }
}

region *rrandneighbour(region * r)
{
    direction_t i;
    region *rc = NULL;
    int rr, c = 0;

    /* Nachsehen, wieviele Regionen in Frage kommen */

    for (i = 0; i != MAXDIRECTIONS; i++) {
        c++;
    }
    /* Zuf�llig eine ausw�hlen */

    rr = rng_int() % c;

    /* Durchz�hlen */

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
        /* F�r 6-17 Runden */
        a->data.sa[1] = (short)(a->data.sa[1] + time);
    }

    /* Personen bekommen 4W10 Punkte Schaden. */

    for (up = &r->units; *up;) {
        unit *u = *up;
        if (u->number) {
            int dead = damage_unit(u, damage, true, false);
            /* TODO create undead */
            if (dead) {
                ADDMSG(&u->faction->msgs, msg_message("volcano_dead",
                    "unit region dead", u, volcano, dead));
            }
            if (!fval(u->faction, FFL_SELECT)) {
                fset(u->faction, FFL_SELECT);
                ADDMSG(&u->faction->msgs, msg_message("volcanooutbreaknn",
                    "region", r));
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

void volcano_outbreak(region * r)
{
    region *rn;
    unit *u;
    faction *f;

    for (f = NULL, u = r->units; u; u = u->next) {
        if (f != u->faction) {
            f = u->faction;
            freset(f, FFL_SELECT);
        }
    }

    /* Zuf�llige Nachbarregion verw�sten */
    rn = rrandneighbour(r);
    volcano_destruction(r, r, "4d10");
    if (rn) {
        volcano_destruction(r, rn, "3d10");
    }
}

static void melt_iceberg(region * r)
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
    terraform_region(r, newterrain(T_OCEAN));

    /* Einheiten, die nicht schwimmen k�nnen oder in Schiffen sind,
     * ertrinken */
    drown(r);
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
                    if (ship_isdestroyed(sh)) {
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

    for (r = regions; r; r = r->next) {
        if (r->terrain == newterrain(T_ICEBERG) && !fval(r, RF_SELECT)) {
            int select = rng_int() % 10;
            if (select < 4) {
                /* 4% chance */
                fset(r, RF_SELECT);
                melt_iceberg(r);
            }
            else if (select < 64) {
                /* 60% chance */
                fset(r, RF_SELECT);
                move_iceberg(r);
            }
        }
    }
}

void create_icebergs(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        if (r->terrain == newterrain(T_ICEBERG_SLEEP) && chance(0.05)) {
            bool has_ocean_neighbour = false;
            direction_t dir;
            region *rc;
            unit *u;

            freset(r, RF_SELECT);
            for (dir = 0; dir < MAXDIRECTIONS; dir++) {
                rc = rconnect(r, dir);
                if (rc && fval(rc->terrain, SEA_REGION)) {
                    has_ocean_neighbour = true;
                    break;
                }
            }
            if (!has_ocean_neighbour)
                continue;

            rsetterrain(r, T_ICEBERG);

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
        if (is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
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
                    if (ship_isdestroyed(sh)) {
                        unit *u = ship_owner(sh);
                        if (u)
                            ADDMSG(&u->faction->msgs,
                            msg_message("godcurse_destroy_ship", "ship", sh));
                        remove_ship(&sh->region->ships, sh);
                    }
                    sh = shn;
                }
            }
        }
    }

}

/** handles the "orcish" curse that makes units grow like old orks
 * This would probably be better handled in an age-function for the curse,
 * but it's now being called by randomevents()
 */
static void orc_growth(void)
{
    region *r;
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            static bool init = false;
            static const curse_type *ct_orcish = 0;
            curse *c = 0;
            if (!init) {
                init = true;
                ct_orcish = ct_find("orcish");
            }
            if (ct_orcish)
                c = get_curse(u->attribs, ct_orcish);

            if (c && !has_skill(u, SK_MAGIC) && !has_skill(u, SK_ALCHEMY)
                && !fval(u, UFL_HERO)) {
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

/** Talente von D�monen verschieben sich.
 */
static void demon_skillchanges(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (u_race(u) == get_race(RC_DAEMON)) {
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
    region *r;
    create_icebergs();
    move_icebergs();
    for (r = regions; r; r = r->next) {
        drown(r);
    }
}

#define HERBS_ROT               /* herbs owned by units have a chance to rot. */
#define HERBROTCHANCE 5         /* Verrottchance f�r Kr�uter (ifdef HERBS_ROT) */
#ifdef HERBS_ROT
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
                    double nv = normalvariate(k, k / 4);
                    int inv = (int)nv;
                    int delta = _min(n, inv);
                    if (!i_change(itmp, itm->type, -delta)) {
                        continue;
                    }
                }
                itmp = &itm->next;
            }
        }
    }
}
#endif

void randomevents(void)
{
    region *r;
    faction *monsters = get_monsters();

    icebergs();
    godcurse();
    orc_growth();
    demon_skillchanges();

    /* Orkifizierte Regionen mutieren und mutieren zur�ck */

    for (r = regions; r; r = r->next) {
        if (fval(r, RF_ORCIFIED)) {
            direction_t dir;
            double probability = 0.0;
            for (dir = 0; dir < MAXDIRECTIONS; dir++) {
                region *rc = rconnect(r, dir);
                if (rc && rpeasants(rc) > 0 && !fval(rc, RF_ORCIFIED))
                    probability += 0.02;
            }
            if (chance(probability)) {
                ADDMSG(&r->msgs, msg_message("deorcified", "region", r));
                freset(r, RF_ORCIFIED);
            }
        }
        else {
            attrib *a = a_find(r->attribs, &at_orcification);
            if (a != NULL) {
                double probability = 0.0;
                if (rpeasants(r) <= 0)
                    continue;
                probability = a->data.i / (double)rpeasants(r);
                if (chance(probability)) {
                    fset(r, RF_ORCIFIED);
                    a_remove(&r->attribs, a);
                    ADDMSG(&r->msgs, msg_message("orcified", "region", r));
                }
                else {
                    a->data.i -= _max(10, a->data.i / 10);
                    if (a->data.i <= 0)
                        a_remove(&r->attribs, a);
                }
            }
        }
    }

    /* Vulkane qualmen, brechen aus ... */
    for (r = regions; r; r = r->next) {
        if (r->terrain == newterrain(T_VOLCANO_SMOKING)) {
            if (a_find(r->attribs, &at_reduceproduction)) {
                ADDMSG(&r->msgs, msg_message("volcanostopsmoke", "region", r));
                rsetterrain(r, T_VOLCANO);
            }
            else {
                if (rng_int() % 100 < 12) {
                    ADDMSG(&r->msgs, msg_message("volcanostopsmoke", "region", r));
                    rsetterrain(r, T_VOLCANO);
                }
                else if (r->age > 20 && rng_int() % 100 < 8) {
                    volcano_outbreak(r);
                    rsetterrain(r, T_VOLCANO);
                }
            }
        }
        else if (r->terrain == newterrain(T_VOLCANO)) {
            if (rng_int() % 100 < 4) {
                ADDMSG(&r->msgs, msg_message("volcanostartsmoke", "region", r));
                rsetterrain(r, T_VOLCANO_SMOKING);
            }
        }
    }

    /* Monumente zerfallen, Schiffe verfaulen */

    for (r = regions; r; r = r->next) {
        building **blist = &r->buildings;
        while (*blist) {
            building *b = *blist;
            if (fval(b->type, BTF_DECAY) && !building_owner(b)) {
                b->size -= _max(1, (b->size * 20) / 100);
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
        for (r = regions; r; r = r->next) {
            unit *u;

            for (u = r->units; u; u = u->next) {
                if (u->faction && !is_monsters(u->faction)
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

    chaos_update();
#ifdef HERBS_ROT
    rotting_herbs();
#endif

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
