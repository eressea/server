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

#ifdef _MSC_VER
#include <platform.h>
#endif
#include <kernel/config.h>
#include "laws.h"

#include <modules/gmcmd.h>

#include "alchemy.h"
#include "battle.h"
#include "economy.h"
#include "keyword.h"
#include "market.h"
#include "morale.h"
#include "monsters.h"
#include "move.h"
#include "randenc.h"
#include "renumber.h"
#include "spy.h"
#include "study.h"
#include "wormhole.h"
#include "prefix.h"
#include "reports.h"
#include "teleport.h"
#include "guard.h"
#include "volcano.h"

/* kernel includes */
#include "kernel/alliance.h"
#include "kernel/ally.h"
#include "kernel/calendar.h"
#include "kernel/callbacks.h"
#include "kernel/connection.h"
#include "kernel/curse.h"
#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/group.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/plane.h"
#include "kernel/pool.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"
#include "kernel/terrain.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/parser.h>
#include <util/password.h>
#include <util/path.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/strings.h>
#include <util/umlaut.h>
#include <util/unicode.h>

/* attributes includes */
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/seenspell.h>
#include <attributes/stealth.h>

#include <spells/buildingcurse.h>
#include <spells/regioncurse.h>
#include <spells/unitcurse.h>

#include <selist.h>
#include <iniparser.h>

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* chance that a peasant dies of starvation: */
#define PEASANT_STARVATION_CHANCE 0.9
/* Pferdevermehrung */
#define HORSEGROWTH 4
/* Wanderungschance pro Pferd */
#define HORSEMOVE   3
/* Vermehrungschance pro Baum */
#define FORESTGROWTH 10000      /* In Millionstel */

/** Ausbreitung und Vermehrung */
#define MAXDEMAND      25
#define DMRISE         0.1F     /* weekly chance that demand goes up */
#define DMRISEHAFEN    0.2F     /* weekly chance that demand goes up with harbor */

/* - exported global symbols ----------------------------------- */

int NewbieImmunity(void)
{
    int result = config_get_int("NewbieImmunity", 0);
    return result;
}

bool IsImmune(const faction * f)
{
    return !fval(f, FFL_NPC) && f->age < NewbieImmunity();
}

int NMRTimeout(void)
{
    int nmr_timeout = config_get_int("nmr.timeout", 0);
    int ini_timeout = config_get_int("game.maxnmr", 0);
    if (nmr_timeout > 0) {
        if (ini_timeout > nmr_timeout) {
            return nmr_timeout;
        }
        return (ini_timeout > 0) ? ini_timeout : nmr_timeout;
    }
    return ini_timeout;
}

bool LongHunger(const struct unit *u)
{
    if (u != NULL) {
        if (!fval(u, UFL_HUNGER))
            return false;
        if (u_race(u) == get_race(RC_DAEMON))
            return false;
    }
    return config_get_int("hunger.long", 0) != 0;
}

static bool RemoveNMRNewbie(void)
{
    int value = config_get_int("nmr.removenewbie", 0);
    return value != 0;
}

static void dumbeffect(unit *u) {
    int effect = get_effect(u, oldpotiontype[P_FOOL]);
    if (effect > 0) {           /* Trank "Dumpfbackenbrot" */
        skill *sv = u->skills, *sb = NULL;
        while (sv != u->skills + u->skill_size) {
            if (sb == NULL || skill_compare(sv, sb) > 0) {
                sb = sv;
            }
            ++sv;
        }
        /* bestes Talent raussuchen */
        if (sb != NULL) {
            int weeks = u->number;
            if (weeks > effect) weeks = effect;
            reduce_skill(u, sb, weeks);
            ADDMSG(&u->faction->msgs, msg_message("dumbeffect",
                "unit weeks skill", u, weeks, (skill_t)sb->id));
        }                         /* sonst Glück gehabt: wer nix weiss, kann nix vergessen... */
        change_effect(u, oldpotiontype[P_FOOL], -effect);
    }
}

static void astral_crumble(unit *u) {
    item **itemp = &u->items;
    while (*itemp) {
        item *itm = *itemp;
        if ((itm->type->flags & ITF_NOTLOST) == 0) {
            if (itm->type->flags & (ITF_BIG | ITF_ANIMAL | ITF_CURSED)) {
                ADDMSG(&u->faction->msgs, msg_message("itemcrumble",
                    "unit region item amount",
                    u, u->region, itm->type->rtype, itm->number));
                i_free(i_remove(itemp, itm));
                continue;
            }
        }
        itemp = &itm->next;
    }
}

static void age_unit(region * r, unit * u)
{
    const race *rc = u_race(u);

    ++u->age;
    if (u->number > 0 && rc->age_unit) {
        rc->age_unit(u);
    }
    if (u->attribs) {
        dumbeffect(u);
    }
    if (u->items && u->region && is_astral(u->region)) {
        astral_crumble(u);
    }
}

static void live(region * r)
{
    unit **up = &r->units;

    get_food(r);

    while (*up) {
        unit *u = *up;
        /* IUW: age_unit() kann u loeschen, u->next ist dann
         * undefiniert, also muessen wir hier schon das nächste
         * Element bestimmen */
        age_unit(r, u);
        if (*up == u)
            up = &u->next;
    }
}

/*
 * This procedure calculates the number of emigrating peasants for the given
 * region r. There are two incentives for peasants to emigrate:
 * 1) They prefer the less crowded areas.
 *    Example: mountains, 700 peasants (max  1000), 70% inhabited
 *             plain, 5000 peasants (max 10000), 50% inhabited
 *             700*(PEASANTSWANDER_WEIGHT/100)*((70-50)/100) peasants wander
 *             from mountains to plain.
 *    Effect : peasents will leave densely populated regions.
 * 2) Peasants prefer richer neighbour regions.
 *    Example: region A,  700 peasants, wealth $10500, $15 per head
 *             region B, 2500 peasants, wealth $25000, $10 per head
 *             Some peasants will emigrate from B to A because $15 > $10
 *             exactly: 2500*(PEASANTSGREED_WEIGHT1/100)*((15-10)/100)
 * Not taken in consideration:
 * - movement because of monsters.
 * - movement because of wars
 * - movement because of low loyalty relating to present parties.
 */

#define MAX_EMIGRATION(p) ((p)/MAXDIRECTIONS)
#define MAX_IMMIGRATION(p) ((p)*2/3)

static void calculate_emigration(region * r)
{
    int i;
    int maxp = region_maxworkers(r);
    int rp = rpeasants(r);
    int max_immigrants = MAX_IMMIGRATION(maxp - rp);


    if (volcano_module()) {
        static int terrain_cache;
        static const terrain_type *t_volcano;
        static const terrain_type *t_smoking;

        if (terrain_changed(&terrain_cache)) {
            t_volcano = newterrain(T_VOLCANO);
            t_smoking = newterrain(T_VOLCANO_SMOKING);
        }
        if (r->terrain == t_volcano || r->terrain == t_smoking) {
            max_immigrants = max_immigrants / 10;
        }
    }

    for (i = 0; max_immigrants > 0 && i != MAXDIRECTIONS; i++) {
        int dir = (turn + 1 + i) % MAXDIRECTIONS;
        region *rc = rconnect(r, (direction_t)dir);

        if (rc != NULL && fval(rc->terrain, LAND_REGION)) {
            int rp2 = rpeasants(rc);
            int maxp2 = region_maxworkers(rc);
            int max_emigration = MAX_EMIGRATION(rp2 - maxp2);

            if (max_emigration > 0) {
                if (max_emigration > max_immigrants) max_emigration = max_immigrants;
                r->land->newpeasants += max_emigration;
                rc->land->newpeasants -= max_emigration;
                max_immigrants -= max_emigration;
            }
        }
    }
}


static double peasant_growth_factor(void)
{
    return config_get_flt("rules.peasants.growth.factor", 0.0001F * PEASANTGROWTH);
}

static double peasant_luck_factor(void)
{
    return config_get_flt("rules.peasants.peasantluck.factor", PEASANTLUCK);
}

int peasant_luck_effect(int peasants, int luck, int maxp, double variance)
{
    int births = 0;
    double mean;
    if (luck == 0) return 0;
    mean = fmin(luck, peasants);
    mean *= peasant_luck_factor() * peasant_growth_factor();
    mean *= ((peasants / (double)maxp < .9) ? 1 : PEASANTFORCE);

    births = RAND_ROUND(normalvariate(mean, variance * mean));
    if (births <= 0)
        births = 1;
    if (births > peasants / 2)
        births = peasants / 2 + 1;
    return births;
}

static void peasants(region * r, int rule)
{
    int rp = rpeasants(r);
    int money = rmoney(r);
    int maxp = max_production(r);
    int n, satiated;
    int dead = 0, peasants = rp;

    if (peasants > 0 && rule > 0) {
        int luck = 0;
        double fraction = peasants * peasant_growth_factor();
        int births = RAND_ROUND(fraction);
        attrib *a = a_find(r->attribs, &at_peasantluck);

        if (a != NULL) {
            luck = a->data.i * 1000;
        }

        luck = peasant_luck_effect(peasants, luck, maxp, .5);
        if (luck > 0)
            ADDMSG(&r->msgs, msg_message("peasantluck_success", "births", luck));
        peasants += births + luck;
    }

    /* Alle werden satt, oder halt soviele für die es auch Geld gibt */

    satiated = money / maintenance_cost(NULL);
    if (satiated > peasants) satiated = peasants;
    rsetmoney(r, money - satiated * maintenance_cost(NULL));

    /* Von denjenigen, die nicht satt geworden sind, verhungert der
     * Großteil. dead kann nie größer als rpeasants(r) - satiated werden,
     * so dass rpeasants(r) >= 0 bleiben muß. */

     /* Es verhungert maximal die unterernährten Bevölkerung. */

    n = peasants - satiated;
    if (n > rp) n = rp;
    dead += (int)(0.5 + n * PEASANT_STARVATION_CHANCE);

    if (dead > 0) {
        message *msg = add_message(&r->msgs, msg_message("phunger", "dead", dead));
        msg_release(msg);
        peasants -= dead;
    }

    rsetpeasants(r, peasants);
}

/* ------------------------------------------------------------- */

typedef struct migration {
    struct migration *next;
    region *r;
    int horses;
    int trees;
} migration;

#define MSIZE 1023
migration *migrants[MSIZE];
migration *free_migrants;

static migration *get_migrants(region * r)
{
    int key = reg_hashkey(r);
    int index = key % MSIZE;
    migration *m = migrants[index];
    while (m && m->r != r)
        m = m->next;
    if (m == NULL) {
        /* Es gibt noch keine Migration. Also eine erzeugen
         */
        m = free_migrants;
        if (!m)
            m = calloc(1, sizeof(migration));
        else {
            free_migrants = free_migrants->next;
            m->horses = 0;
            m->trees = 0;
        }
        m->r = r;
        m->next = migrants[index];
        migrants[index] = m;
    }
    return m;
}

static void migrate(region * r)
{
    int key = reg_hashkey(r);
    int index = key % MSIZE;
    migration **hp = &migrants[index];
    fset(r, RF_MIGRATION);
    while (*hp && (*hp)->r != r)
        hp = &(*hp)->next;
    if (*hp) {
        migration *m = *hp;
        rsethorses(r, rhorses(r) + m->horses);
        /* Was macht das denn hier?
         * Baumwanderung wird in trees() gemacht.
         * wer fragt das? Die Baumwanderung war abhängig von der
         * Auswertungsreihenfolge der regionen,
         * das hatte ich geändert. jemand hat es wieder gelöscht, toll.
         * ich habe es wieder aktiviert, muss getestet werden.
         */
        *hp = m->next;
        m->next = free_migrants;
        free_migrants = m;
    }
}

static void horses(region * r)
{
    int horses, maxhorses;
    direction_t n;

    /* Logistisches Wachstum, Optimum bei halbem Maximalbesatz. */
    maxhorses = region_maxworkers(r) / 10;
    horses = rhorses(r);
    if (horses > 0) {
        if (is_cursed(r->attribs, &ct_godcursezone)) {
            rsethorses(r, (int)(horses * 0.9));
        }
        else if (maxhorses > 0) {
            double growth =
                (RESOURCE_QUANTITY * HORSEGROWTH * 200 * (maxhorses -
                    horses)) / maxhorses;

            if (growth > 0) {
                int i;
                if (a_find(r->attribs, &at_horseluck)) {
                    growth *= 2;
                }
                /* printf("Horses: <%d> %d -> ", growth, horses); */
                i = (int)(0.5 + (horses * 0.0001) * growth);
                /* printf("%d\n", horses); */
                rsethorses(r, horses + i);
            }
        }
    }

    /* Pferde wandern in Nachbarregionen.
     * Falls die Nachbarregion noch berechnet
     * werden muss, wird eine migration-Struktur gebildet,
     * die dann erst in die Berechnung der Nachbarstruktur einfließt.
     */

    for (n = 0; n != MAXDIRECTIONS; n++) {
        region *r2 = rconnect(r, n);
        if (r2 && fval(r2->terrain, WALK_INTO)) {
            int pt = (rhorses(r) * HORSEMOVE) / 100;
            pt = (int)normalvariate(pt, pt / 4.0);
            if (pt < 0) pt = 0;
            if (fval(r2, RF_MIGRATION))
                rsethorses(r2, rhorses(r2) + pt);
            else {
                migration *nb;
                /* haben wir die Migration schonmal benutzt?
                 * wenn nicht, müssen wir sie suchen.
                 * Wandernde Pferde vermehren sich nicht.
                 */
                nb = get_migrants(r2);
                nb->horses += pt;
            }
            /* Wandernde Pferde sollten auch abgezogen werden */
            rsethorses(r, rhorses(r) - pt);
        }
    }
    assert(rhorses(r) >= 0);
}

static int count_race(const region * r, const race * rc)
{
    unit *u;
    int c = 0;

    for (u = r->units; u; u = u->next)
        if (u_race(u) == rc)
            c += u->number;

    return c;
}

attrib_type at_germs = {
    "germs",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writeshorts,
    a_readshorts,
    NULL,
    ATF_UNIQUE
};


static void
growing_trees_e3(region * r, const int current_season,
    const int last_weeks_season)
{
    static const int transform[4][3] = {
        { -1, -1, 0 },
        { TREE_SEED, TREE_SAPLING, 2 },
        { TREE_SAPLING, TREE_TREE, 2 },
        { TREE_TREE, TREE_SEED, 2 }
    };

    if (r->land && current_season != last_weeks_season
        && transform[current_season][2]) {
        int src_type = transform[current_season][0];
        int dst_type = transform[current_season][1];
        int src = rtrees(r, src_type);
        int dst = rtrees(r, dst_type);
        int grow = src / transform[current_season][2];
        if (grow > 0) {
            if (src_type != TREE_TREE) {
                rsettrees(r, src_type, src - grow);
            }
            rsettrees(r, dst_type, dst + grow);

            if (dst_type == TREE_SEED && r->terrain->size) {
                region *rn[MAXDIRECTIONS];
                int d;
                double fgrow = grow / (double)MAXDIRECTIONS;

                get_neighbours(r, rn);
                for (d = 0; d != MAXDIRECTIONS; ++d) {
                    region *rx = rn[d];
                    if (rx && rx->land) {
                        double scale = 1.0;
                        int g;
                        double fg, ch;
                        int seeds = rtrees(rx, dst_type);

                        if (r->terrain->size > rx->terrain->size) {
                            scale = (scale * rx->terrain->size) / r->terrain->size;
                        }
                        fg = scale * fgrow;
                        g = (int)fg;
                        ch = fg - g;
                        if (chance(ch))
                            ++g;
                        if (g > 0) {
                            rsettrees(rx, dst_type, seeds + g);
                        }
                    }
                }
            }
        }
    }
}

static void
growing_trees(region * r, const int current_season, const int last_weeks_season)
{
    int grownup_trees, i, seeds, sprout;
    attrib *a;

    if (current_season == SEASON_SUMMER || current_season == SEASON_AUTUMN) {
        double seedchance = 0.01F * RESOURCE_QUANTITY;
        int mp, elves = count_race(r, get_race(RC_ELF));
        direction_t d;

        a = a_find(r->attribs, &at_germs);
        if (a && last_weeks_season == SEASON_SPRING) {
            /* ungekeimte Samen bleiben erhalten, Sprößlinge wachsen */
            sprout = rtrees(r, 1);
            if (sprout > a->data.sa[1]) sprout = a->data.sa[1];
            /* aus dem gesamt Sprößlingepool abziehen */
            rsettrees(r, 1, rtrees(r, 1) - sprout);
            /* zu den Bäumen hinzufügen */
            rsettrees(r, 2, rtrees(r, 2) + sprout);

            a_removeall(&r->attribs, &at_germs);
        }

        if (is_cursed(r->attribs, &ct_godcursezone)) {
            rsettrees(r, 1, (int)(rtrees(r, 1) * 0.9));
            rsettrees(r, 2, (int)(rtrees(r, 2) * 0.9));
            return;
        }

        mp = max_production(r);
        if (mp <= 0)
            return;

        /* Grundchance 1.0% */
        /* Jeder Elf in der Region erhöht die Chance marginal */
        mp = mp / 8;
        if (elves > mp) elves = mp;
        if (elves) {
            seedchance += 1.0 - pow(0.99999, elves * RESOURCE_QUANTITY);
        }
        grownup_trees = rtrees(r, 2);
        seeds = 0;

        if (grownup_trees > 0) {
            double remainder = seedchance * grownup_trees;
            seeds = (int)(remainder);
            remainder -= seeds;
            if (chance(remainder)) {
                ++seeds;
            }
            if (seeds > 0) {
                seeds += rtrees(r, 0);
                rsettrees(r, 0, seeds);
            }
        }

        /* Bäume breiten sich in Nachbarregionen aus. */

        /* Gesamtzahl der Samen:
         * bis zu 6% (FORESTGROWTH*3) der Bäume samen in die Nachbarregionen */
        seeds = (rtrees(r, 2) * FORESTGROWTH * 3) / 1000000;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *r2 = rconnect(r, d);
            if (r2 && fval(r2->terrain, LAND_REGION) && r2->terrain->size) {
                /* Eine Landregion, wir versuchen Samen zu verteilen:
                 * Die Chance, das Samen ein Stück Boden finden, in dem sie
                 * keimen können, hängt von der Bewuchsdichte und der
                 * verfügbaren Fläche ab. In Gletschern gibt es weniger
                 * Möglichkeiten als in Ebenen. */
                sprout = 0;
                seedchance = (1000.0 * region_maxworkers(r2)) / r2->terrain->size;
                for (i = 0; i < seeds / MAXDIRECTIONS; i++) {
                    if (rng_int() % 10000 < seedchance)
                        sprout++;
                }
                rsettrees(r2, 0, rtrees(r2, 0) + sprout);
            }
        }

    }
    else if (current_season == SEASON_SPRING) {
        int growth;

        if (is_cursed(r->attribs, &ct_godcursezone))
            return;

        /* in at_germs merken uns die Zahl der Samen und Sprößlinge, die
         * dieses Jahr älter werden dürfen, damit nicht ein Same im selben
         * Zyklus zum Baum werden kann */
        a = a_find(r->attribs, &at_germs);
        if (!a) {
            a = a_add(&r->attribs, a_new(&at_germs));
            a->data.sa[0] = (short)rtrees(r, 0);
            a->data.sa[1] = (short)rtrees(r, 1);
        }
        /* wir haben 6 Wochen zum wachsen, jeder Same/Spross hat 18% Chance
         * zu wachsen, damit sollten nach 5-6 Wochen alle gewachsen sein */
        growth = 1800;

        /* Samenwachstum */

        /* Raubbau abfangen, es dürfen nie mehr Samen wachsen, als aktuell
         * in der Region sind */
        seeds = rtrees(r, 0);
        if (seeds > a->data.sa[0]) seeds = a->data.sa[0];
        sprout = 0;

        for (i = 0; i < seeds; i++) {
            if (rng_int() % 10000 < growth)
                sprout++;
        }
        /* aus dem Samenpool dieses Jahres abziehen */
        a->data.sa[0] = (short)(seeds - sprout);
        /* aus dem gesamt Samenpool abziehen */
        rsettrees(r, 0, rtrees(r, 0) - sprout);
        /* zu den Sprößlinge hinzufügen */
        rsettrees(r, 1, rtrees(r, 1) + sprout);

        /* Baumwachstum */

        /* hier gehen wir davon aus, das Jungbäume nicht ohne weiteres aus
         * der Region entfernt werden können, da Jungbäume in der gleichen
         * Runde nachwachsen, wir also nicht mehr zwischen diesjährigen und
         * 'alten' Jungbäumen unterscheiden könnten */
        sprout = rtrees(r, 1);
        if (sprout > a->data.sa[1]) sprout = a->data.sa[1];
        grownup_trees = 0;

        for (i = 0; i < sprout; i++) {
            if (rng_int() % 10000 < growth)
                grownup_trees++;
        }
        /* aus dem Sprößlingepool dieses Jahres abziehen */
        a->data.sa[1] = (short)(sprout - grownup_trees);
        /* aus dem gesamt Sprößlingepool abziehen */
        rsettrees(r, 1, rtrees(r, 1) - grownup_trees);
        /* zu den Bäumen hinzufügen */
        rsettrees(r, 2, rtrees(r, 2) + grownup_trees);
    }
}

static void
growing_herbs(region * r, const int current_season, const int last_weeks_season)
{
    /* Jetzt die Kräutervermehrung. Vermehrt wird logistisch:
     *
     * Jedes Kraut hat eine Wahrscheinlichkeit von (100-(vorhandene
     * Kräuter))% sich zu vermehren. */
    UNUSED_ARG(last_weeks_season);
    if (current_season != SEASON_WINTER) {
        int i;
        for (i = rherbs(r); i > 0; i--) {
            if (rng_int() % 100 < (100 - rherbs(r)))
                rsetherbs(r, (short)(rherbs(r) + 1));
        }
    }
}

void immigration(void)
{
    region *r;
    int repopulate = config_get_int("rules.economy.repopulate_maximum", 90);
    log_info(" - Einwanderung...");
    for (r = regions; r; r = r->next) {
        if (r->land && r->land->newpeasants) {
            int rp = rpeasants(r) + r->land->newpeasants;
            /* FIXME: kann ernsthaft abs(newpeasants) > rpeasants(r) sein? */
            if (rp < 0) rp = 0;
            rsetpeasants(r, rp);
        }
        /* Genereate some (0-6 depending on the income) peasants out of nothing */
        /* if less than 50 are in the region and there is space and no monster or demon units in the region */
        if (repopulate) {
            int peasants = rpeasants(r);
            int income = wage(r, NULL, NULL, turn) - maintenance_cost(NULL) + 1;
            if (income >= 0 && r->land && (peasants < repopulate) && region_maxworkers(r) >(peasants + 30) * 2) {
                int badunit = 0;
                unit *u;
                for (u = r->units; u; u = u->next) {
                    if (!playerrace(u_race(u)) || u_race(u) == get_race(RC_DAEMON)) {
                        badunit = 1;
                        break;
                    }
                }
                if (badunit == 0) {
                    peasants += (int)(rng_double()*income);
                    rsetpeasants(r, peasants);
                }
            }
        }
    }
}

void nmr_warnings(void)
{
    faction *f, *fa;
#define FRIEND (HELP_GUARD|HELP_MONEY)
    for (f = factions; f; f = f->next) {
        if (!fval(f, FFL_NOIDLEOUT) && turn > f->lastorders) {
            ADDMSG(&f->msgs, msg_message("nmr_warning", ""));
            if (turn - f->lastorders == NMRTimeout() - 1) {
                ADDMSG(&f->msgs, msg_message("nmr_warning_final", ""));
            }
            if ((turn - f->lastorders) >= 2) {
                message *msg = NULL;
                for (fa = factions; fa; fa = fa->next) {
                    int warn = 0;
                    if (config_get_int("rules.alliances", 0) != 0) {
                        if (f->alliance && f->alliance == fa->alliance) {
                            warn = 1;
                        }
                    }
                    else if (alliedfaction(NULL, f, fa, FRIEND)
                        && alliedfaction(NULL, fa, f, FRIEND)) {
                        warn = 1;
                    }
                    if (warn) {
                        if (msg == NULL) {
                            msg =
                                msg_message("warn_dropout", "faction turns", f,
                                    turn - f->lastorders);
                        }
                        add_message(&fa->msgs, msg);
                    }
                }
                if (msg != NULL)
                    msg_release(msg);
            }
        }
    }
}

void demographics(void)
{
    region *r;
    static int last_weeks_season = -1;
    static int current_season = -1;
    int plant_rules = config_get_int("rules.grow.formula", 2);
    int horse_rules = config_get_int("rules.horses.growth", 1);
    int peasant_rules = config_get_int("rules.peasants.growth", 1);
    const struct building_type *bt_harbour = bt_find("harbour");

    if (current_season < 0) {
        gamedate date;
        get_gamedate(turn, &date);
        current_season = date.season;
        get_gamedate(turn - 1, &date);
        last_weeks_season = date.season;
    }

    for (r = regions; r; r = r->next) {
        ++r->age; /* also oceans. no idea why we didn't always do that */
        live(r);

        if (!fval(r->terrain, SEA_REGION)) {
            /* die Nachfrage nach Produkten steigt. */
            struct demand *dmd;
            if (r->land) {
                for (dmd = r->land->demands; dmd; dmd = dmd->next) {
                    if (dmd->value > 0 && dmd->value < MAXDEMAND) {
                        float rise = DMRISE;
                        if (buildingtype_exists(r, bt_harbour, true))
                            rise = DMRISEHAFEN;
                        if (rng_double() < rise)
                            ++dmd->value;
                    }
                }
                /* Seuchen erst nachdem die Bauern sich vermehrt haben
                 * und gewandert sind */

                calculate_emigration(r);
                peasants(r, peasant_rules);

                if (r->age > 20) {
                    double mwp = fmax(region_maxworkers(r), 1);
                    double prob =
                        pow(rpeasants(r) / (mwp * wage(r, NULL, NULL, turn) * 0.13), 4.0)
                        * PLAGUE_CHANCE;

                    if (rng_double() < prob) {
                        plagues(r);
                    }
                }
                if (horse_rules > 0) {
                    horses(r);
                }
                if (plant_rules == 2) { /* E2 */
                    growing_trees(r, current_season, last_weeks_season);
                    growing_herbs(r, current_season, last_weeks_season);
                }
                else if (plant_rules==1) { /* E3 */
                    growing_trees_e3(r, current_season, last_weeks_season);
                }
            }

            update_resources(r);
            if (r->land)
                migrate(r);
        }
    }
    while (free_migrants) {
        migration *m = free_migrants->next;
        free(free_migrants);
        free_migrants = m;
    };

    remove_empty_units();
    immigration();
}

/* ------------------------------------------------------------- */

/* test if the unit can slip through a siege undetected.
 * returns 0 if siege is successful, or 1 if the building is either
 * not besieged or the unit can slip through the siege due to better stealth.
 */
static int slipthru(const region * r, const unit * u, const building * b)
{
    unit *u2;
    int n, o;

    /* b ist die burg, in die man hinein oder aus der man heraus will. */
    if (b == NULL || b->besieged < b->size * SIEGEFACTOR) {
        return 1;
    }

    /* u wird am hinein- oder herausschluepfen gehindert, wenn STEALTH <=
     * OBSERVATION +2 der belagerer u2 ist */
    n = effskill(u, SK_STEALTH, r);

    for (u2 = r->units; u2; u2 = u2->next) {
        if (usiege(u2) == b) {

            if (invisible(u, u2) >= u->number)
                continue;

            o = effskill(u2, SK_PERCEPTION, r);

            if (o + 2 >= n) {
                return 0;               /* entdeckt! */
            }
        }
    }
    return 1;
}

int can_contact(const region * r, const unit * u, const unit * u2) {

    /* hier geht es nur um die belagerung von burgen */
    UNUSED_ARG(r);
    if (u->building == u2->building) {
        return 1;
    }

    /* unit u is trying to contact u2 - unasked for contact. wenn u oder u2
     * nicht in einer burg ist, oder die burg nicht belagert ist, ist
     * slipthru () == 1. ansonsten ist es nur 1, wenn man die belagerer */

    if (slipthru(u->region, u, u->building) && slipthru(u->region, u2, u2->building)) {
        return 1;
    }

    return (alliedunit(u, u2->faction, HELP_GIVE));
}

int contact_cmd(unit * u, order * ord)
{
    unit *u2;
    int n;

    init_order_depr(ord);
    n = read_unitid(u->faction, u->region);
    u2 = findunit(n);

    if (u2 != NULL) {
        if (!can_contact(u->region, u, u2)) {
            cmistake(u, u->thisorder, 23, MSG_EVENT);
            return -1;
        }
        usetcontact(u, u2);
    }
    return 0;
}

int leave_cmd(unit * u, struct order *ord)
{
    region *r = u->region;

    if (fval(u, UFL_ENTER)) {
        /* if we just entered this round, then we don't leave again */
        return 0;
    }

    if (fval(r->terrain, SEA_REGION) && u->ship) {
        if (!fval(u_race(u), RCF_SWIM)) {
            cmistake(u, ord, 11, MSG_MOVE);
            return 0;
        }
        if (has_horses(u)) {
            cmistake(u, ord, 231, MSG_MOVE);
            return 0;
        }
    }
    if (!slipthru(r, u, u->building)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder, "entrance_besieged",
            "building", u->building));
    }
    else {
        leave(u, true);
    }
    return 0;
}

int quit_cmd(unit * u, struct order *ord)
{
    char token[128];
    faction *f = u->faction;
    const char *passwd;
    keyword_t kwd;

    kwd = init_order_depr(ord);
    assert(kwd == K_QUIT);
    passwd = gettoken(token, sizeof(token));
    if (checkpasswd(f, (const char *)passwd)) {
        fset(f, FFL_QUIT);
    }
    else {
        char buffer[64];
        write_order(ord, f->locale, buffer, sizeof(buffer));
        cmistake(u, ord, 86, MSG_EVENT);
        log_warning("QUIT with illegal password for faction %s: %s\n", itoa36(f->no), buffer);
    }
    return 0;
}

static bool mayenter(region * r, unit * u, building * b)
{
    unit *u2;
    UNUSED_ARG(r);
    if (fval(b, BLD_UNGUARDED))
        return true;
    u2 = building_owner(b);

    if (u2 == NULL || ucontact(u2, u)
        || alliedunit(u2, u->faction, HELP_GUARD))
        return true;

    return false;
}

static int mayboard(const unit * u, ship * sh)
{
    unit *u2 = ship_owner(sh);

    return (!u2 || ucontact(u2, u) || alliedunit(u2, u->faction, HELP_GUARD));
}

static bool CheckOverload(void)
{
    return config_get_int("rules.check_overload", 0) != 0;
}

int enter_ship(unit * u, struct order *ord, int id, bool report)
{
    region *r = u->region;
    ship *sh;
    const race * rc = u_race(u);

    /* Muss abgefangen werden, sonst koennten Schwimmer an
     * Bord von Schiffen an Land gelangen. */
    if (!(rc->flags & (RCF_CANSAIL | RCF_WALK | RCF_FLY))) {
        if (report) {
            cmistake(u, ord, 233, MSG_MOVE);
        }
        return 0;
    }

    sh = findship(id);
    if (sh == NULL || sh->region != r) {
        if (report) {
            cmistake(u, ord, 20, MSG_MOVE);
        }
        return 0;
    }
    if (sh == u->ship) {
        return 1;
    }
    if (!mayboard(u, sh)) {
        if (report) {
            cmistake(u, ord, 34, MSG_MOVE);
        }
        return 0;
    }
    if (CheckOverload()) {
        int sweight, scabins;
        int mweight = shipcapacity(sh);
        int mcabins = sh->type->cabins;

        if (mweight > 0) {
            getshipweight(sh, &sweight, &scabins);
            sweight += weight(u);
            if (mcabins) {
                int pweight = u->number * u_race(u)->weight;
                /* weight goes into number of cabins, not cargo */
                scabins += pweight;
                sweight -= pweight;
            }

            if (sweight > mweight || (mcabins && (scabins > mcabins))) {
                if (report)
                    cmistake(u, ord, 34, MSG_MOVE);
                return 0;
            }
        }
    }

    if (leave(u, false)) {
        u_set_ship(u, sh);
        fset(u, UFL_ENTER);
        return 1;
    }
    else if (report) {
        cmistake(u, ord, 150, MSG_MOVE);
    }
    return 0;
}

int enter_building(unit * u, order * ord, int id, bool report)
{
    region *r = u->region;
    building *b;

    /* Schwimmer können keine Gebäude betreten, außer diese sind
     * auf dem Ozean */
    if (!fval(u_race(u), RCF_WALK) && !fval(u_race(u), RCF_FLY)) {
        if (!fval(r->terrain, SEA_REGION)) {
            if (report) {
                cmistake(u, ord, 232, MSG_MOVE);
            }
            return 0;
        }
    }

    b = findbuilding(id);
    if (b == NULL || b->region != r) {
        if (report) {
            cmistake(u, ord, 6, MSG_MOVE);
        }
        return 0;
    }
    if (!mayenter(r, u, b)) {
        if (report) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "entrance_denied",
                "building", b));
        }
        return 0;
    }
    if (!slipthru(r, u, b)) {
        if (report) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "entrance_besieged",
                "building", b));
        }
        return 0;
    }

    if (leave(u, 0)) {
        fset(u, UFL_ENTER);
        u_set_building(u, b);
        return 1;
    }
    else if (report) {
        cmistake(u, ord, 150, MSG_MOVE);
    }
    return 0;
}

static void do_contact(region * r)
{
    unit * u;
    for (u = r->units; u; u = u->next) {
        order *ord;
        for (ord = u->orders; ord; ord = ord->next) {
            keyword_t kwd = getkeyword(ord);
            if (kwd == K_CONTACT) {
                contact_cmd(u, ord);
            }
        }
    }
}

void do_enter(struct region *r, bool is_final_attempt)
{
    unit **uptr;

    for (uptr = &r->units; *uptr;) {
        unit *u = *uptr;
        order **ordp = &u->orders;

        while (*ordp) {
            order *ord = *ordp;
            if (getkeyword(ord) == K_ENTER) {
                char token[128];
                param_t p;
                int id;
                unit *ulast = NULL;
                const char * s;

                init_order_depr(ord);
                s = gettoken(token, sizeof(token));
                p = findparam_ex(s, u->faction->locale);
                id = getid();

                switch (p) {
                case P_BUILDING:
                case P_GEBAEUDE:
                    if (u->building && u->building->no == id)
                        break;
                    if (enter_building(u, ord, id, is_final_attempt)) {
                        unit *ub;
                        for (ub = u; ub; ub = ub->next) {
                            if (ub->building == u->building) {
                                ulast = ub;
                            }
                        }
                    }
                    break;

                case P_SHIP:
                    if (u->ship && u->ship->no == id)
                        break;
                    if (enter_ship(u, ord, id, is_final_attempt)) {
                        unit *ub;
                        ulast = u;
                        for (ub = u; ub; ub = ub->next) {
                            if (ub->ship == u->ship) {
                                ulast = ub;
                            }
                        }
                    }
                    break;

                default:
                    if (is_final_attempt) {
                        cmistake(u, ord, 79, MSG_MOVE);
                    }
                }
                if (ulast != NULL) {
                    /* Wenn wir hier angekommen sind, war der Befehl
                     * erfolgreich und wir löschen ihn, damit er im
                     * zweiten Versuch nicht nochmal ausgeführt wird. */
                    *ordp = ord->next;
                    ord->next = NULL;
                    free_order(ord);

                    if (ulast != u) {
                        /* put u behind ulast so it's the last unit in the building */
                        *uptr = u->next;
                        u->next = ulast->next;
                        ulast->next = u;
                    }
                    break;
                }
            }
            if (*ordp == ord)
                ordp = &ord->next;
        }
        if (*uptr == u)
            uptr = &u->next;
    }
}

int dropouts[2];
int *age = NULL;

static void nmr_death(faction * f)
{
    int rule = config_get_int("rules.nmr.destroy", 0) != 0;
    if (rule) {
        unit *u;
        for (u = f->units; u; u = u->nextF) {
            if (u->building && building_owner(u->building) == u) {
                remove_building(&u->region->buildings, u->building);
            }
        }
    }
}

static void remove_idle_players(void)
{
    faction **fp;
    int i, timeout = NMRTimeout();

    log_info(" - beseitige Spieler, die sich zu lange nicht mehr gemeldet haben...");

    for (fp = &factions; *fp;) {
        faction *f = *fp;

        if (timeout > 0 && turn - f->lastorders >= timeout) {
            nmr_death(f);
            destroyfaction(fp);
        } else {
            if (fval(f, FFL_NOIDLEOUT)) {
                f->lastorders = turn;
            }
            else if (turn != f->lastorders) {
                char info[256];
                sprintf(info, "%d Einheiten, %d Personen",
                    f->num_units, f->num_people);
            }
            fp = &f->next;
        }
    }
    log_info(" - beseitige Spieler, die sich nach der Anmeldung nicht gemeldet haben...");

    i = turn + 1;
    if (i < 4) i = 4;
    age = calloc(i, sizeof(int));
    for (fp = &factions; *fp;) {
        faction *f = *fp;
        if (!is_monsters(f)) {
            if (RemoveNMRNewbie() && !fval(f, FFL_NOIDLEOUT)) {
                if (f->age >= 0 && f->age <= turn)
                    ++age[f->age];
                if (f->age == 2 || f->age == 3) {
                    if (f->lastorders == turn - 2) {
                        ++dropouts[f->age - 2];
                        destroyfaction(fp);
                        continue;
                    }
                }
            }
        }
        fp = &f->next;
    }
}

void quit(void)
{
    faction **fptr = &factions;
    while (*fptr) {
        faction *f = *fptr;
        if (f->flags & FFL_QUIT) {
            destroyfaction(fptr);
        }
        else {
            ++f->age;
            fptr = &f->next;
        }
    }
    remove_idle_players();
    remove_empty_units();
}

/* ------------------------------------------------------------- */

/* HELFE partei [<ALLES | SILBER | GIB | KAEMPFE | WAHRNEHMUNG>] [NICHT] */

int ally_cmd(unit * u, struct order *ord)
{
    char token[128];
    ally *sf, **sfp;
    faction *f;
    int keyword, not_kw;
    const char *s;

    init_order_depr(ord);
    f = getfaction();

    if (f == NULL || is_monsters(f)) {
        cmistake(u, ord, 66, MSG_EVENT);
        return 0;
    }
    if (f == u->faction)
        return 0;

    s = gettoken(token, sizeof(token));

    if (!s || !s[0]) {
        keyword = P_ANY;
    }
    else {
        keyword = findparam(s, u->faction->locale);
    }

    sfp = &u->faction->allies;
    if (fval(u, UFL_GROUP)) {
        attrib *a = a_find(u->attribs, &at_group);
        if (a)
            sfp = &((group *)a->data.v)->allies;
    }
    for (sf = *sfp; sf; sf = sf->next)
        if (sf->faction == f)
            break;                    /* Gleich die passende raussuchen, wenn vorhanden */

    not_kw = getparam(u->faction->locale);        /* HELFE partei [modus] NICHT */

    if (!sf) {
        if (keyword == P_NOT || not_kw == P_NOT) {
            /* Wir helfen der Partei gar nicht... */
            return 0;
        }
        else {
            sf = ally_add(sfp, f);
            sf->status = 0;
        }
    }
    switch (keyword) {
    case P_NOT:
        sf->status = 0;
        break;

    case NOPARAM:
        cmistake(u, ord, 137, MSG_EVENT);
        return 0;

    case P_ANY:
        if (not_kw == P_NOT)
            sf->status = 0;
        else
            sf->status = HELP_ALL;
        break;

    case P_TRAVEL:
        if (not_kw == P_NOT)
            sf->status = sf->status & (HELP_ALL - HELP_TRAVEL);
        else
            sf->status = sf->status | HELP_TRAVEL;
        break;

    case P_GIVE:
        if (not_kw == P_NOT)
            sf->status = sf->status & (HELP_ALL - HELP_GIVE);
        else
            sf->status = sf->status | HELP_GIVE;
        break;

    case P_MONEY:
        if (not_kw == P_NOT)
            sf->status = sf->status & (HELP_ALL - HELP_MONEY);
        else
            sf->status = sf->status | HELP_MONEY;
        break;

    case P_FIGHT:
        if (not_kw == P_NOT)
            sf->status = sf->status & (HELP_ALL - HELP_FIGHT);
        else
            sf->status = sf->status | HELP_FIGHT;
        break;

    case P_FACTIONSTEALTH:
        if (not_kw == P_NOT)
            sf->status = sf->status & (HELP_ALL - HELP_FSTEALTH);
        else
            sf->status = sf->status | HELP_FSTEALTH;
        break;

    case P_GUARD:
        if (not_kw == P_NOT)
            sf->status = sf->status & (HELP_ALL - HELP_GUARD);
        else
            sf->status = sf->status | HELP_GUARD;
        break;
    }

    sf->status &= HelpMask();

    if (sf->status == 0) {        /* Alle HELPs geloescht */
        removelist(sfp, sf);
    }
    return 0;
}

static struct local_names *pnames;

static void init_prefixnames(void)
{
    int i;
    for (i = 0; localenames[i]; ++i) {
        const struct locale *lang = get_locale(localenames[i]);
        bool exist = false;
        struct local_names *in = pnames;

        while (in != NULL) {
            if (in->lang == lang) {
                exist = true;
                break;
            }
            in = in->next;
        }
        if (in == NULL)
            in = calloc(sizeof(local_names), 1);
        in->next = pnames;
        in->lang = lang;

        if (!exist && race_prefixes) {
            int key;
            for (key = 0; race_prefixes[key]; ++key) {
                variant var;
                const char *pname =
                    LOC(lang, mkname("prefix", race_prefixes[key]));
                if (findtoken(in->names, pname, &var) == E_TOK_NOMATCH || var.i != key) {
                    var.i = key;
                    addtoken((struct tnode **)&in->names, pname, var);
                    addtoken((struct tnode **)&in->names, LOC(lang, mkname("prefix",
                        race_prefixes[key])), var);
                }
            }
        }
        pnames = in;
    }
}

int prefix_cmd(unit * u, struct order *ord)
{
    char token[128];
    attrib **ap;
    const char *s;
    local_names *in = pnames;
    variant var;
    const struct locale *lang = u->faction->locale;

    while (in != NULL) {
        if (in->lang == lang)
            break;
        in = in->next;
    }
    if (in == NULL) {
        init_prefixnames();
        for (in = pnames; in->lang != lang; in = in->next);
    }

    init_order_depr(ord);
    s = gettoken(token, sizeof(token));

    if (!s || !*s) {
        attrib *a = NULL;
        if (fval(u, UFL_GROUP)) {
            a = a_find(u->attribs, &at_group);
        }
        if (a) {
            group *g = (group *)a->data.v;
            a_removeall(&g->attribs, &at_raceprefix);
        }
        else {
            a_removeall(&u->faction->attribs, &at_raceprefix);
        }
        return 0;
    }
    else if (findtoken(in->names, s, &var) == E_TOK_NOMATCH) {
        return 0;
    }
    else if (race_prefixes[var.i] == NULL) {
        cmistake(u, ord, 299, MSG_EVENT);
    }
    else {
        ap = &u->faction->attribs;
        if (fval(u, UFL_GROUP)) {
            attrib *a = a_find(u->attribs, &at_group);
            if (a) {
                group *g = (group *)a->data.v;
                ap = &g->attribs;
            }
        }
        set_prefix(ap, race_prefixes[var.i]);
    }
    return 0;
}

int display_cmd(unit * u, struct order *ord)
{
    char token[128];
    char **s = NULL;
    const char *str;
    region *r = u->region;

    init_order_depr(ord);

    str = gettoken(token, sizeof(token));
    switch (findparam_ex(str, u->faction->locale)) {
    case P_BUILDING:
    case P_GEBAEUDE:
        if (!u->building) {
            cmistake(u, ord, 145, MSG_PRODUCE);
            break;
        }
        if (building_owner(u->building) != u) {
            cmistake(u, ord, 5, MSG_PRODUCE);
            break;
        }
        if (!fval(u->building->type, BTF_NAMECHANGE) && u->building->display && u->building->display[0]) {
            cmistake(u, ord, 278, MSG_EVENT);
            break;
        }
        s = &u->building->display;
        break;

    case P_SHIP:
        if (!u->ship) {
            cmistake(u, ord, 144, MSG_PRODUCE);
            break;
        }
        if (ship_owner(u->ship) != u) {
            cmistake(u, ord, 12, MSG_PRODUCE);
            break;
        }
        s = &u->ship->display;
        break;

    case P_UNIT:
        s = &u->display;
        break;

    case P_PRIVAT:
        usetprivate(u, getstrtoken());
        break;

    case P_REGION:
        if (!r->land || u->faction != region_get_owner(r)) {
            cmistake(u, ord, 147, MSG_EVENT);
            break;
        }
        s = &r->land->display;
        break;

    default:
        cmistake(u, ord, 110, MSG_EVENT);
        break;
    }

    if (s != NULL) {
        const char *s2 = getstrtoken();

        free(*s);
        if (s2) {
            char * sdup = str_strdup(s2);
            if (unicode_utf8_trim(sdup) != 0) {
                log_info("trimming info: %s", s2);
            }
            if (strlen(sdup) >= DISPLAYSIZE) {
                sdup[DISPLAYSIZE-1] = 0;
            }
            *s = sdup;
        }
        else {
            *s = NULL;
        }
    }

    return 0;
}

bool renamed_building(const building * b)
{
    const struct locale *lang = locales;
    size_t len = strlen(b->name);
    for (; lang; lang = nextlocale(lang)) {
        const char *bdname = LOC(lang, b->type->_name);
        if (bdname) {
            size_t bdlen = strlen(bdname);
            if (len >= bdlen && strncmp(b->name, bdname, bdlen) == 0) {
                return false;
            }
        }
    }
    return true;
}

static int rename_cmd(unit * u, order * ord, char **s, const char *s2)
{
	char name[NAMESIZE];
    assert(s2);
    if (!s2[0]) {
        cmistake(u, ord, 84, MSG_EVENT);
        return 0;
    }

    /* TODO: Validate to make sure people don't have illegal characters in
     * names, phishing-style? () come to mind. */
    str_strlcpy(name, s2, sizeof(name));
    if (unicode_utf8_trim(name) != 0) {
        log_info("trimming name: %s", s2);
    }

    free(*s);
    *s = str_strdup(name);
    return 0;
}

static bool try_rename(unit *u, building *b, order *ord) {
    unit *owner = b ? building_owner(b) : 0;
    bool foreign = !(owner && owner->faction == u->faction);

    if (!b) {
        cmistake(u, ord, u->building ? 6 : 145, MSG_EVENT);
        return false;
    }

    if (!fval(b->type, BTF_NAMECHANGE) && renamed_building(b)) {
        cmistake(u, ord, 278, MSG_EVENT);
        return false;
    }

    if (foreign) {
        if (renamed_building(b)) {
            cmistake(u, ord, 246, MSG_EVENT);
            return false;
        }

        if (owner) {
            if (cansee(owner->faction, u->region, u, 0)) {
                ADDMSG(&owner->faction->msgs,
                    msg_message("renamed_building_seen",
                        "building renamer region", b, u, u->region));
            }
            else {
                ADDMSG(&owner->faction->msgs,
                    msg_message("renamed_building_notseen",
                        "building region", b, u->region));
            }
            if (owner != u) {
                cmistake(u, ord, 148, MSG_PRODUCE);
                return false;
            }
        }
    }
    return true;
}

int
rename_building(unit * u, order * ord, building * b, const char *name)
{
    assert(name);
    if (!try_rename(u, b, ord)) {
        return -1;
    }
    return rename_cmd(u, ord, &b->name, name);
}

int name_cmd(struct unit *u, struct order *ord)
{
    char token[128];
    building *b = u->building;
    region *r = u->region;
    char **s = NULL;
    param_t p;
    bool foreign = false;
    const char *str;

    init_order_depr(ord);
    str = gettoken(token, sizeof(token));
    p = findparam_ex(str, u->faction->locale);

    if (p == P_FOREIGN) {
        str = gettoken(token, sizeof(token));
        foreign = true;
        p = findparam_ex(str, u->faction->locale);
    }

    switch (p) {
    case P_ALLIANCE:
        if (!foreign && f_get_alliance(u->faction)) {
            alliance *al = u->faction->alliance;
            faction *lead = alliance_get_leader(al);
            if (lead == u->faction) {
                s = &al->name;
            }
        }
        break;
    case P_BUILDING:
    case P_GEBAEUDE:
        if (foreign) {
            b = getbuilding(u->region);
        }
        if (try_rename(u, b, ord)) {
            s = &b->name;
        }
        break;
    case P_FACTION:
        if (foreign) {
            faction *f;

            f = getfaction();
            if (!f) {
                cmistake(u, ord, 66, MSG_EVENT);
                break;
            }
            if (f->age < 10) {
                cmistake(u, ord, 248, MSG_EVENT);
                break;
            }
            else {
                const struct locale *lang = locales;
                size_t f_len = strlen(f->name);
                for (; lang; lang = nextlocale(lang)) {
                    const char *fdname = LOC(lang, "factiondefault");
                    size_t fdlen = strlen(fdname);
                    if (f_len >= fdlen && strncmp(f->name, fdname, fdlen) == 0) {
                        break;
                    }
                }
                if (lang == NULL) {
                    cmistake(u, ord, 247, MSG_EVENT);
                    break;
                }
            }
            if (cansee(f, r, u, 0)) {
                ADDMSG(&f->msgs,
                    msg_message("renamed_faction_seen", "unit region", u, r));
            }
            else {
                ADDMSG(&f->msgs, msg_message("renamed_faction_notseen", "", r));
            }
            s = &f->name;
        }
        else {
            s = &u->faction->name;
        }
        break;

    case P_SHIP:
        if (foreign) {
            ship *sh = getship(r);
            unit *uo;

            if (!sh) {
                cmistake(u, ord, 20, MSG_EVENT);
                break;
            }
            else {
                const struct locale *lang = locales;
                size_t sh_len = strlen(sh->name);
                for (; lang; lang = nextlocale(lang)) {
                    const char *sdname = LOC(lang, sh->type->_name);
                    size_t sdlen = strlen(sdname);
                    if (sh_len >= sdlen && strncmp(sh->name, sdname, sdlen) == 0) {
                        break;
                    }

                    sdname = LOC(lang, parameters[P_SHIP]);
                    sdlen = strlen(sdname);
                    if (sh_len >= sdlen && strncmp(sh->name, sdname, sdlen) == 0) {
                        break;
                    }

                }
                if (lang == NULL) {
                    cmistake(u, ord, 245, MSG_EVENT);
                    break;
                }
            }
            uo = ship_owner(sh);
            if (uo) {
                if (cansee(uo->faction, r, u, 0)) {
                    ADDMSG(&uo->faction->msgs,
                        msg_message("renamed_ship_seen", "ship renamer region", sh, u, r));
                }
                else {
                    ADDMSG(&uo->faction->msgs,
                        msg_message("renamed_ship_notseen", "ship region", sh, r));
                }
            }
            s = &sh->name;
        }
        else {
            unit *uo;
            if (!u->ship) {
                cmistake(u, ord, 144, MSG_PRODUCE);
                break;
            }
            uo = ship_owner(u->ship);
            if (uo->faction != u->faction) {
                cmistake(u, ord, 12, MSG_PRODUCE);
                break;
            }
            s = &u->ship->name;
        }
        break;

    case P_UNIT:
        if (foreign) {
            unit *u2 = 0;

            getunit(r, u->faction, &u2);
            if (!u2 || !cansee(u->faction, r, u2, 0)) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
                    "feedback_unit_not_found", ""));
                break;
            }
            else {
                char udefault[32];
                default_name(u2, udefault, sizeof(udefault));
                if (strcmp(unit_getname(u2), udefault) != 0) {
                    cmistake(u, ord, 244, MSG_EVENT);
                    break;
                }
            }
            if (cansee(u2->faction, r, u, 0)) {
                ADDMSG(&u2->faction->msgs, msg_message("renamed_seen",
                    "renamer renamed region", u, u2, r));
            }
            else {
                ADDMSG(&u2->faction->msgs, msg_message("renamed_notseen",
                    "renamed region", u2, r));
            }
            s = &u2->_name;
        }
        else {
            s = &u->_name;
        }
        break;

    case P_REGION:
        if (u->faction != region_get_owner(r)) {
            cmistake(u, ord, 147, MSG_EVENT);
            break;
        }
        s = &r->land->name;
        break;

    case P_GROUP:
    {
        attrib *a = NULL;
        if (fval(u, UFL_GROUP))
            a = a_find(u->attribs, &at_group);
        if (a) {
            group *g = (group *)a->data.v;
            s = &g->name;
            break;
        }
        else {
            cmistake(u, ord, 109, MSG_EVENT);
            break;
        }
    }
    break;
    default:
        cmistake(u, ord, 109, MSG_EVENT);
        break;
    }

    if (s != NULL) {
        const char *name = getstrtoken();
        if (name) {
            rename_cmd(u, ord, s, name);
        }
        else {
            cmistake(u, ord, 84, MSG_EVENT);
        }
    }

    return 0;
}

/* ------------------------------------------------------------- */

void
deliverMail(faction * f, region * r, unit * u, const char *s, unit * receiver)
{
    if (!cansee(f, r, u, 0)) {
        u = NULL;
    }
    if (!receiver) {              /* BOTSCHAFT an PARTEI */
        ADDMSG(&f->msgs,
            msg_message("regionmessage", "region sender string", r, u, s));
    }
    else {                      /* BOTSCHAFT an EINHEIT */
        ADDMSG(&f->msgs,
            msg_message("unitmessage", "region unit sender string", r,
                receiver, u, s));
    }
}

static void
mailunit(region * r, unit * u, int n, struct order *ord, const char *s)
{
    unit *u2 = findunitr(r, n);

    if (u2 && cansee(u->faction, r, u2, 0)) {
        deliverMail(u2->faction, r, u, s, u2);
        /* now done in prepare_mail_cmd */
    }
    else {
        /* Immer eine Meldung - sonst koennte man so getarnte EHs enttarnen:
         * keine Meldung -> EH hier. */
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "feedback_unit_not_found", ""));
    }
}

static void mailfaction(unit * u, int n, struct order *ord, const char *s)
{
    faction *f;

    f = findfaction(n);
    if (f && n > 0)
        deliverMail(f, u->region, u, s, NULL);
    else
        cmistake(u, ord, 66, MSG_MESSAGE);
}

int mail_cmd(unit * u, struct order *ord)
{
    char token[128];
    region *r = u->region;
    unit *u2;
    const char *s;
    int n, cont;

    init_order_depr(ord);
    s = gettoken(token, sizeof(token));

    /* Falls kein Parameter, ist das eine Einheitsnummer;
     * das Füllwort "AN" muss wegfallen, da gültige Nummer! */

    do {
        cont = 0;
        switch (findparam_ex(s, u->faction->locale)) {
        case P_REGION:
            /* können alle Einheiten in der Region sehen */
            s = getstrtoken();
            if (!s || !s[0]) {
                cmistake(u, ord, 30, MSG_MESSAGE);
                break;
            }
            else {
                ADDMSG(&r->msgs, msg_message("mail_result", "unit message", u, s));
                return 0;
            }

        case P_FACTION:
            n = getfactionid();

            for (u2 = r->units; u2; u2 = u2->next) {
                if (u2->faction->no == n && seefaction(u->faction, r, u2, 0)) {
                    break;
                }
            }

            if (!u2) {
                cmistake(u, ord, 66, MSG_MESSAGE);
                break;
            }

            s = getstrtoken();
            if (!s || !s[0]) {
                cmistake(u, ord, 30, MSG_MESSAGE);
                break;
            }
            mailfaction(u, n, ord, s);
            return 0;

        case P_UNIT:
            n = getid();

            for (u2 = r->units; u2; u2 = u2->next) {
                if (u2->no == n && cansee(u->faction, r, u2, 0)) {
                    break;
                }
            }

            if (!u2) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
                    "feedback_unit_not_found", ""));
                return 0;
            }

            s = getstrtoken();
            if (!s || !s[0]) {
                cmistake(u, ord, 30, MSG_MESSAGE);
                break;
            }
            else {
                attrib *a = a_find(u2->attribs, &at_eventhandler);
                if (a != NULL) {
                    event_arg args[3];
                    args[0].data.v = (void *)s;
                    args[0].type = "string";
                    args[1].data.v = (void *)u;
                    args[1].type = "unit";
                    args[2].type = NULL;
                    handle_event(a, "message", args);
                }

                mailunit(r, u, n, ord, s);
            }
            return 0;

        case P_BUILDING:
        case P_GEBAEUDE:
        {
            building *b = getbuilding(r);

            if (!b) {
                cmistake(u, ord, 6, MSG_MESSAGE);
                break;
            }

            s = getstrtoken();

            if (!s || !s[0]) {
                cmistake(u, ord, 30, MSG_MESSAGE);
                break;
            }

            for (u2 = r->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);

            for (u2 = r->units; u2; u2 = u2->next) {
                if (u2->building == b && !fval(u2->faction, FFL_SELECT)
                    && cansee(u->faction, r, u2, 0)) {
                    mailunit(r, u, u2->no, ord, s);
                    fset(u2->faction, FFL_SELECT);
                }
            }
            return 0;
        }

        case P_SHIP:
        {
            ship *sh = getship(r);

            if (!sh) {
                cmistake(u, ord, 20, MSG_MESSAGE);
                break;
            }

            s = getstrtoken();

            if (!s || !s[0]) {
                cmistake(u, ord, 30, MSG_MESSAGE);
                break;
            }

            for (u2 = r->units; u2; u2 = u2->next)
                freset(u2->faction, FFL_SELECT);

            for (u2 = r->units; u2; u2 = u2->next) {
                if (u2->ship == sh && !fval(u2->faction, FFL_SELECT)
                    && cansee(u->faction, r, u2, 0)) {
                    mailunit(r, u, u2->no, ord, s);
                    fset(u2->faction, FFL_SELECT);
                }
            }
            return 0;
        }

        default:
            /* possibly filler token? */
            s = getstrtoken();
            if (s && *s)
                cont = 1;
            break;
        }
    } while (cont);
    cmistake(u, ord, 149, MSG_MESSAGE);
    return 0;
}

/* ------------------------------------------------------------- */

int banner_cmd(unit * u, struct order *ord)
{
    const char * s;

    free(u->faction->banner);
    init_order_depr(ord);
    s = getstrtoken();
    u->faction->banner = s ? str_strdup(s) : 0;
    add_message(&u->faction->msgs, msg_message("changebanner", "value",
        u->faction->banner));

    return 0;
}

int email_cmd(unit * u, struct order *ord)
{
    const char *s;

    init_order_depr(ord);
    s = getstrtoken();

    if (!s || !s[0]) {
        cmistake(u, ord, 85, MSG_EVENT);
    }
    else {
        faction *f = u->faction;
        if (check_email(s) == 0) {
          faction_setemail(f, s);
          ADDMSG(&f->msgs, msg_message("changemail", "value", faction_getemail(f)));
        } else {
            log_error("Invalid email address for faction %s: %s\n", itoa36(f->no), s);
            ADDMSG(&f->msgs, msg_message("changemail_invalid", "value", s));
        }
    }
    return 0;
}

int password_cmd(unit * u, struct order *ord)
{
    char pwbuf[32];
    const char *s;
    bool pwok = true;

    init_order_depr(ord);
    s = gettoken(pwbuf, sizeof(pwbuf));

    if (!s || !*s) {
        int i;
        for (i = 0; i < 6; i++)
            pwbuf[i] = (char)(97 + rng_int() % 26);
        pwbuf[6] = 0;
    }
    else {
        char *c;
        for (c = pwbuf; *c && pwok; ++c) {
            if (!isalnum(*(unsigned char *)c)) {
                pwok = false;
            }
        }
    }
    if (!pwok) {
        cmistake(u, ord, 283, MSG_EVENT);
        str_strlcpy(pwbuf, itoa36(rng_int()), sizeof(pwbuf));
    }
    faction_setpassword(u->faction, password_encode(pwbuf, PASSWORD_DEFAULT));
    ADDMSG(&u->faction->msgs, msg_message("changepasswd",
        "value", pwbuf));
    u->faction->flags |= FFL_PWMSG;
    return 0;
}

int send_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *s;
    int option;

    init_order_depr(ord);
    s = gettoken(token, sizeof(token));

    option = findoption(s, u->faction->locale);

    if (option == -1) {
        cmistake(u, ord, 135, MSG_EVENT);
    }
    else {
        if (getparam(u->faction->locale) == P_NOT) {
            if (option == O_COMPRESS || option == O_BZIP2) {
                cmistake(u, ord, 305, MSG_EVENT);
            }
            else {
                u->faction->options = u->faction->options & ~(1 << option);
            }
        }
        else {
            u->faction->options = u->faction->options | (1 << option);
            if (option == O_COMPRESS)
                u->faction->options &= ~(1 << O_BZIP2);
            if (option == O_BZIP2)
                u->faction->options &= ~(1 << O_COMPRESS);
        }
    }
    return 0;
}

static void display_potion(unit * u, const item_type * itype) {
    show_item(u, itype);
}

static void display_item(unit * u, const item_type * itype)
{
    faction * f = u->faction;
    const char *name;
    const char *key;
    const char *info;

    name = resourcename(itype->rtype, 0);
    key = mkname("iteminfo", name);
    info = locale_getstring(f->locale, key);

    if (info == NULL) {
        info = LOC(f->locale, mkname("iteminfo", "no_info"));
    }
    ADDMSG(&f->msgs, msg_message("displayitem", "weight item description",
        itype->weight, itype->rtype, info));
}

static void display_race(faction * f, const race * rc)
{
    char buf[2048];

    report_raceinfo(rc, f->locale, buf, sizeof(buf));
    addmessage(0, f, buf, MSG_EVENT, ML_IMPORTANT);
}

static void reshow_other(unit * u, struct order *ord, const char *s) {
    int err = 21;
    bool found = false;

    if (s) {
        const spell *sp = 0;
        const item_type *itype;
        const race *rc;
        /* check if it's an item */
        itype = finditemtype(s, u->faction->locale);
        sp = unit_getspell(u, s, u->faction->locale);
        rc = findrace(s, u->faction->locale);

        if (itype) {
            /* if this is a potion, we need the right alchemy skill */
            err = 36; /* we do not have this item? */
            if (itype->flags & ITF_POTION) {
                /* we don't have the item, but it is a potion. do we know it? */
                int level = potion_level(itype);
                if (level > 0 && 2 * level <= effskill(u, SK_ALCHEMY, 0)) {
                    display_potion(u, itype);
                    found = true;
                }
            }
            else {
                int i = i_get(u->items, itype);
                if (i > 0) {
                    found = true;
                    display_item(u, itype);
                }
            }
        }

        if (sp) {
            reset_seen_spells(u->faction, sp);
            found = true;
        }

        if (rc && u_race(u) == rc) {
            display_race(u->faction, rc);
            found = true;
        }
    }
    if (!found) {
        cmistake(u, ord, err, MSG_EVENT);
    }
}

static void reshow(unit * u, struct order *ord, const char *s, param_t p)
{
    switch (p) {
    case P_ZAUBER:
        reset_seen_spells(u->faction, NULL);
        break;
    case P_POTIONS:
        if (!display_potions(u)) {
            cmistake(u, ord, 285, MSG_EVENT);
        }
        break;
    case NOPARAM:
        reshow_other(u, ord, s);
        break;
    default:
        cmistake(u, ord, 222, MSG_EVENT);
        break;
    }
}

int promotion_cmd(unit * u, struct order *ord)
{
    const struct resource_type *rsilver = get_resourcetype(R_SILVER);
    int money, people;

    if (fval(u, UFL_HERO)) {
        /* TODO: message "is already a hero" */
        return 0;
    }

    if (maxheroes(u->faction) < countheroes(u->faction) + u->number) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "heroes_maxed", "max count",
                maxheroes(u->faction), countheroes(u->faction)));
        return 0;
    }
    if (!valid_race(u->faction, u_race(u))) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "heroes_race", "race",
            u_race(u)));
        return 0;
    }
    people = u->faction->num_people * u->number;
    money = get_pooled(u, rsilver, GET_ALL, people);

    if (people > money) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "heroes_cost", "cost have", people, money));
        return 0;
    }
    use_pooled(u, rsilver, GET_ALL, people);
    fset(u, UFL_HERO);
    ADDMSG(&u->faction->msgs, msg_message("hero_promotion", "unit cost",
        u, people));
    return 0;
}

int group_cmd(unit * u, struct order *ord)
{
    keyword_t kwd;

    kwd = init_order_depr(ord);
    assert(kwd == K_GROUP);
    join_group(u, getstrtoken());
    return 0;
}

int origin_cmd(unit * u, struct order *ord)
{
    int px, py;

    init_order_depr(ord);

    px = getint();
    py = getint();

    faction_setorigin(u->faction, getplaneid(u->region), px, py);
    return 0;
}

int guard_off_cmd(unit * u, struct order *ord)
{
    assert(getkeyword(ord) == K_GUARD);
    init_order_depr(ord);

    if (getparam(u->faction->locale) == P_NOT) {
        setguard(u, false);
    }
    return 0;
}

int reshow_cmd(unit * u, struct order *ord)
{
    char lbuf[64];
    const char *s;
    param_t p = NOPARAM;

    init_order_depr(ord);
    s = gettoken(lbuf, sizeof(lbuf));

    if (s && isparam(s, u->faction->locale, P_ANY)) {
        p = getparam(u->faction->locale);
        s = NULL;
    }

    reshow(u, ord, s, p);
    return 0;
}

int status_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *s;

    init_order_depr(ord);
    s = gettoken(token, sizeof(token));
    switch (findparam(s, u->faction->locale)) {
    case P_NOT:
        unit_setstatus(u, ST_AVOID);
        break;
    case P_BEHIND:
        unit_setstatus(u, ST_BEHIND);
        break;
    case P_FLEE:
        unit_setstatus(u, ST_FLEE);
        break;
    case P_CHICKEN:
        unit_setstatus(u, ST_CHICKEN);
        break;
    case P_AGGRO:
        unit_setstatus(u, ST_AGGRO);
        break;
    case P_VORNE:
        unit_setstatus(u, ST_FIGHT);
        break;
    case P_HELP:
        if (getparam(u->faction->locale) == P_NOT) {
            fset(u, UFL_NOAID);
        }
        else {
            freset(u, UFL_NOAID);
        }
        break;
    default:
        if (s && s[0]) {
            add_message(&u->faction->msgs,
                msg_feedback(u, ord, "unknown_status", ""));
        }
        else {
            unit_setstatus(u, ST_FIGHT);
        }
    }
    return 0;
}

int combatspell_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *s;
    int level = 0;
    spell *sp = 0;

    init_order_depr(ord);
    s = gettoken(token, sizeof(token));

    /* KAMPFZAUBER [NICHT] löscht alle gesetzten Kampfzauber */
    if (!s || *s == 0 || findparam(s, u->faction->locale) == P_NOT) {
        unset_combatspell(u, 0);
        return 0;
    }

    /* Optional: STUFE n */
    if (findparam(s, u->faction->locale) == P_LEVEL) {
        /* Merken, setzen kommt erst später */
        level = getuint();
        s = gettoken(token, sizeof(token));
    }

    sp = unit_getspell(u, s, u->faction->locale);
    if (!sp) {
        cmistake(u, ord, 173, MSG_MAGIC);
        return 0;
    }

    s = gettoken(token, sizeof(token));

    if (findparam(s, u->faction->locale) == P_NOT) {
        /* KAMPFZAUBER "<Spruchname>" NICHT  löscht diesen speziellen
         * Kampfzauber */
        unset_combatspell(u, sp);
        return 0;
    }
    else {
        /* KAMPFZAUBER "<Spruchname>"  setzt diesen Kampfzauber */
        set_combatspell(u, sp, ord, level);
    }

    return 0;
}

int guard_on_cmd(unit * u, struct order *ord)
{
    assert(getkeyword(ord) == K_GUARD);
    assert(u);
    assert(u->faction);

    init_order_depr(ord);

    /* GUARD NOT is handled in goard_off_cmd earlier in the turn */
    if (getparam(u->faction->locale) == P_NOT) {
        return 0;
    }

    if (fval(u, UFL_MOVED)) {
        cmistake(u, ord, 187, MSG_EVENT);
    }
    else if (fval(u_race(u), RCF_ILLUSIONARY)) {
        cmistake(u, ord, 95, MSG_EVENT);
    }
    else {
        int err = can_start_guarding(u);
        if (err == E_GUARD_OK) {
            setguard(u, true);
        }
        else if (err == E_GUARD_TERRAIN) {
            cmistake(u, ord, 2, MSG_EVENT);
        }
        else if (err == E_GUARD_UNARMED) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "unit_unarmed", ""));
        }
        else if (err == E_GUARD_FLEEING) {
            cmistake(u, ord, 320, MSG_EVENT);
        }
        else if (err == E_GUARD_NEWBIE) {
            cmistake(u, ord, 304, MSG_EVENT);
        }
    }
    return 0;
}

void sinkships(struct region * r)
{
    ship **shp = &r->ships;

    while (*shp) {
        ship *sh = *shp;

        if (!sh->type->construction || sh->size >= sh->type->construction->maxsize) {
            if (fval(r->terrain, SEA_REGION)) {
                if (!enoughsailors(sh, crew_skill(sh))) {
                    /* ship is at sea, but not enough people to control it */
                    double dmg = config_get_flt("rules.ship.damage.nocrewocean", 0.3);
                    damage_ship(sh, dmg);
                }
            }
            else if (!ship_owner(sh)) {
                /* any ship lying around without an owner slowly rots */
                double dmg = config_get_flt("rules.ship.damage.nocrew", 0.05);
                damage_ship(sh, dmg);
            }
        }
        if (sh->damage >= sh->size * DAMAGE_SCALE) {
            sink_ship(sh);
            remove_ship(shp, sh);
        }
        if (*shp == sh)
            shp = &sh->next;
    }
}

void restack_units(void)
{
    region *r;
    for (r = regions; r; r = r->next) {
        unit **up = &r->units;
        bool sorted = false;
        while (*up) {
            unit *u = *up;
            if (!fval(u, UFL_MARK)) {
                struct order *ord;
                for (ord = u->orders; ord; ord = ord->next) {
                    if (getkeyword(ord) == K_SORT) {
                        char token[128];
                        const char *s;
                        param_t p;
                        int id;
                        unit *v;

                        init_order_depr(ord);
                        s = gettoken(token, sizeof(token));
                        p = findparam(s, u->faction->locale);
                        id = getid();
                        v = findunit(id);

                        if (!v || v->faction != u->faction || v->region != r) {
                            cmistake(u, ord, 258, MSG_EVENT);
                        }
                        else if (v->building != u->building || v->ship != u->ship) {
                            cmistake(u, ord, 259, MSG_EVENT);
                        }
                        else if (u->building && building_owner(u->building) == u) {
                            cmistake(u, ord, 260, MSG_EVENT);
                        }
                        else if (u->ship && ship_owner(u->ship) == u) {
                            cmistake(u, ord, 260, MSG_EVENT);
                        }
                        else if (v == u) {
                            syntax_error(u, ord);
                        }
                        else {
                            switch (p) {
                            case P_AFTER:
                                *up = u->next;
                                u->next = v->next;
                                v->next = u;
                                fset(u, UFL_MARK);
                                sorted = true;
                                break;
                            case P_BEFORE:
                                if (v->ship && ship_owner(v->ship) == v) {
                                    cmistake(v, ord, 261, MSG_EVENT);
                                }
                                else if (v->building && building_owner(v->building) == v) {
                                    cmistake(v, ord, 261, MSG_EVENT);
                                }
                                else {
                                    unit **vp = &r->units;
                                    while (*vp != v)
                                        vp = &(*vp)->next;
                                    *vp = u;
                                    *up = u->next;
                                    u->next = v;
                                }
                                fset(u, UFL_MARK);
                                sorted = true;
                                break;
                            default:
                                /* TODO: syntax error message? */
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            if (u == *up)
                up = &u->next;
        }
        if (sorted) {
            unit *u;
            for (u = r->units; u; u = u->next) {
                freset(u, UFL_MARK);
            }
        }
    }
}

/* blesses stone circles create an astral protection in the astral region
 * above the shield, which prevents chaos suction and other spells.
 * The shield is created when a magician enters the blessed stone circle,
 * and lasts for as long as his skill level / 2 is, at no mana cost.
 *
 * TODO: this would be nicer in a btype->age function, but we don't have it.
 */
static void age_stonecircle(building *b) {
    const struct resource_type *rtype = get_resourcetype(R_UNICORN);
    region *r = b->region;
    unit *u, *mage = NULL;

    /* step 1: give unicorns to people in the building,
     * find out if there's a magician in there. */
    for (u = r->units; u; u = u->next) {
        if (b == u->building && inside_building(u)) {
            if (!mage && is_mage(u)) {
                mage = u;
            }
            if (rtype) {
                int n, unicorns = 0;
                for (n = 0; n != u->number; ++n) {
                    if (chance(0.02)) {
                        i_change(&u->items, rtype->itype, 1);
                        ++unicorns;
                    }
                    if (unicorns) {
                        ADDMSG(&u->faction->msgs, msg_message("scunicorn",
                                                              "unit amount rtype",
                                                              u, unicorns, rtype));
                    }
                }
            }
        }
    }

    /* step 2: if there's a magician, and a connection to astral space, create the
     * curse. */
    if (get_astralplane()) {
        region *rt = r_standard_to_astral(r);
        if (mage && rt && !fval(rt->terrain, FORBIDDEN_REGION)) {
            curse *c = get_curse(rt->attribs, &ct_astralblock);
            if (!c) {
                int sk = effskill(mage, SK_MAGIC, 0);
                if (sk > 0) {
                    int vig = sk;
                    int dur = (sk + 1) / 2;
                    float effect = 100;
                    /* the mage reactivates the circle */
                    c = create_curse(mage, &rt->attribs, &ct_astralblock,
                        vig, dur, effect, 0);
                    ADDMSG(&r->msgs,
                        msg_message("astralshield_activate", "region unit", r, mage));
                }
            }
            else {
                int sk = effskill(mage, SK_MAGIC, 0);
                if (c->duration < sk / 2) c->duration = sk / 2;
                if (c->vigour < sk) c->vigour = sk;
            }
        }
    }
}

static building *age_building(building * b)
{
    if (is_building_type(b->type, "blessedstonecircle")) {
        age_stonecircle(b);
    }
    a_age(&b->attribs, b);
    handle_event(b->attribs, "timer", b);

    return b;
}

static void age_region(region * r)
{
    a_age(&r->attribs, r);
    handle_event(r->attribs, "timer", r);

    if (!r->land)
        return;

    morale_update(r);
}

static void ageing(void)
{
    faction *f;
    region *r;

    /* altern spezieller Attribute, die eine Sonderbehandlung brauchen?  */
    for (r = regions; r; r = r->next) {
        unit *u;

        for (u = r->units; u; u = u->next) {
            /* Goliathwasser */
            int i = get_effect(u, oldpotiontype[P_STRONG]);
            if (i > 0) {
                if (i > u->number) i = u->number;
                change_effect(u, oldpotiontype[P_STRONG], - i);
            }
            /* Berserkerblut */
            i = get_effect(u, oldpotiontype[P_BERSERK]);
            if (i > 0) {
                if (i > u->number) i = u->number;
                change_effect(u, oldpotiontype[P_BERSERK], - i);
            }

            if (u->attribs) {
                curse * c = get_curse(u->attribs, &ct_oldrace);
                if (c && curse_active(c)) {
                    if (c->duration == 1 && !(c_flags(c) & CURSE_NOAGE)) {
                        u_setrace(u, get_race(curse_geteffect_int(c)));
                        u->irace = NULL;
                    }
                }
            }
        }
    }

    /* Borders */
    age_borders();

    /* Factions */
    for (f = factions; f; f = f->next) {
        a_age(&f->attribs, f);
        handle_event(f->attribs, "timer", f);
    }

    /* Regionen */
    for (r = regions; r; r = r->next) {
        building **bp;
        unit **up;
        ship **sp;

        age_region(r);

        /* Einheiten */
        for (up = &r->units; *up;) {
            unit *u = *up;
            a_age(&u->attribs, u);
            if (u == *up) 
                handle_event(u->attribs, "timer", u);
            if (u == *up) /*-V581 */
                up = &(*up)->next;
        }

        /* Schiffe */
        for (sp = &r->ships; *sp;) {
            ship *s = *sp;
            a_age(&s->attribs, s);
            if (s == *sp)
                handle_event(s->attribs, "timer", s);
            if (s == *sp) /*-V581 */
                sp = &(*sp)->next;
        }

        /* Gebäude */
        for (bp = &r->buildings; *bp;) {
            building *b = *bp;
            age_building(b);
            if (b == *bp)
                bp = &b->next;
        }

        if (rule_region_owners()) {
            update_owners(r);
        }
    }
}

static int maxunits(const faction * f)
{
    int flimit = rule_faction_limit();
    int alimit = rule_alliance_limit();
    UNUSED_ARG(f);
    if (alimit == 0) {
        return flimit;
    }
    if (flimit == 0) {
        return alimit;
    }
    return (alimit > flimit) ? flimit : alimit;
}

int checkunitnumber(const faction * f, int add)
{
    int alimit, flimit;
    int fno = f->num_units + add;
    flimit = rule_faction_limit();
    if (flimit && fno > flimit) {
        return 2;
    }

    alimit = rule_alliance_limit();
    if (alimit) {
        int unitsinalliance = alliance_size(f->alliance);
        if (unitsinalliance + add > alimit) {
            return 1;
        }
    }

    return 0;
}

void maketemp_cmd(unit *u, order **olist) 
{
    order *makeord;
    int err = checkunitnumber(u->faction, 1);

    makeord = *olist;
    if (err) {
        if (err == 1) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, makeord,
                    "too_many_units_in_alliance",
                    "allowed", maxunits(u->faction)));
        }
        else {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, makeord,
                    "too_many_units_in_faction",
                    "allowed", maxunits(u->faction)));
        }
        *olist = makeord->next;
        makeord->next = NULL;
        free_order(makeord);
        while (*olist) {
            keyword_t kwd;
            order * ord = *olist;
            *olist = ord->next;
            ord->next = NULL;
            kwd = getkeyword(ord);
            free_order(ord);
            if (kwd == K_END) {
                break;
            }
        }
    }
    else {
        char token[128];
        const char *s;
        int alias;
        ship *sh;
        unit *u2;
        order **ordp, **oinsert;
#ifndef NDEBUG
        keyword_t kwd = init_order_depr(makeord);
        assert(kwd == K_MAKETEMP);
#endif
        alias = getid();
        s = gettoken(token, sizeof(token));
        if (s && s[0] == '\0') {
            /* empty name? => generate one */
            s = NULL;
        }
        u2 = create_unit(u->region, u->faction, 0, u->faction->race, alias, s, u);
        fset(u2, UFL_ISNEW);
        a_add(&u2->attribs, a_new(&at_alias))->data.i = alias;
        sh = leftship(u);
        if (sh) {
            set_leftship(u2, sh);
        }
        unit_setstatus(u2, u->status);

        /* copy orders until K_END from u to u2 */
        ordp = &makeord->next;
        oinsert = &u2->orders;

        while (*ordp) {
            order *ord = *ordp;
            *ordp = ord->next;
            if (getkeyword(ord) == K_END) {
                ord->next = NULL;
                free_order(ord);
                break;
            }
            *oinsert = ord;
            oinsert = &ord->next;
            *oinsert = NULL;
        }
        *olist = *ordp;
        makeord->next = NULL;
        free_order(makeord);

        if (!u2->orders) {
            order *deford = default_order(u2->faction->locale);
            if (deford) {
                set_order(&u2->thisorder, NULL);
                addlist(&u2->orders, deford);
            }
        }
    }
}

void new_units(void)
{
    region *r;
    unit *u;

    /* neue einheiten werden gemacht und ihre befehle (bis zum "ende" zu
     * ihnen rueberkopiert, damit diese einheiten genauso wie die alten
     * einheiten verwendet werden koennen. */

    for (r = regions; r; r = r->next) {
        for (u = r->units; u; u = u->next) {
            order **ordp = &u->orders;

            /* this needs to happen very early in the game somewhere. since this is
             ** pretty much the first function called per turn, and I am lazy, I
             ** decree that it goes here */
            if (u->flags & UFL_GUARD) {
                fset(r, RF_GUARDED);
            }

            while (*ordp) {
                order *ord = *ordp;
                if (getkeyword(ord) == K_MAKETEMP) {
                    maketemp_cmd(u, ordp);
                }
                else {
                    ordp = &ord->next;
                }
            }
        }
    }
}

void update_long_order(unit * u)
{
    order *ord;
    bool exclusive = true;
    keyword_t thiskwd = NOKEYWORD;
    bool hunger = LongHunger(u);

    freset(u, UFL_MOVED);
    freset(u, UFL_LONGACTION);

    /* check all orders for a potential new long order this round: */
    for (ord = u->orders; ord; ord = ord->next) {
        keyword_t kwd = getkeyword(ord);
        if (kwd == NOKEYWORD) continue;

        if (u->old_orders && is_repeated(kwd)) {
            /* this new order will replace the old defaults */
            free_orders(&u->old_orders);
        }

        /* hungry units do not get long orders: */
        if (hunger) {
            if (u->old_orders) {
                /* keep looking for repeated orders that might clear the old_orders */
                continue;
            }
            break;
        }

        if (is_long(kwd)) {
            if (thiskwd == NOKEYWORD) {
                /* we have found the (first) long order
                 * some long orders can have multiple instances: */
                switch (kwd) {
                    /* Wenn gehandelt wird, darf kein langer Befehl ausgeführt
                     * werden. Da Handel erst nach anderen langen Befehlen kommt,
                     * muss das vorher abgefangen werden. Wir merken uns also
                     * hier, ob die Einheit handelt. */
                case K_BUY:
                case K_SELL:
                case K_CAST:
                    /* non-exclusive orders can be used with others. BUY can be paired with SELL,
                     * CAST with other CAST orders. compatibility is checked once the second
                     * long order is analyzed (below). */
                    exclusive = false;
                    break;

                default:
                    set_order(&u->thisorder, copy_order(ord));
                    break;
                }
                thiskwd = kwd;
            }
            else {
                /* we have found a second long order. this is okay for some, but not all commands.
                 * u->thisorder is already set, and should not have to be updated. */
                switch (kwd) {
                case K_CAST:
                    if (thiskwd != K_CAST) {
                        cmistake(u, ord, 52, MSG_EVENT);
                    }
                    break;
                case K_SELL:
                    if (thiskwd != K_SELL && thiskwd != K_BUY) {
                        cmistake(u, ord, 52, MSG_EVENT);
                    }
                    break;
                case K_BUY:
                    if (thiskwd != K_SELL) {
                        cmistake(u, ord, 52, MSG_EVENT);
                    }
                    else {
                        thiskwd = K_BUY;
                    }
                    break;
                default:
                    if (kwd > thiskwd) {
                        /* swap out thisorder for the new one */
                        cmistake(u, u->thisorder, 52, MSG_EVENT);
                        set_order(&u->thisorder, copy_order(ord));
                    }
                    else {
                        cmistake(u, ord, 52, MSG_EVENT);
                    }
                    break;
                }
            }
        }
    }
    if (hunger) {
        /* Hungernde Einheiten führen NUR den default-Befehl aus */
        set_order(&u->thisorder, default_order(u->faction->locale));
    }
    else if (!exclusive) {
        /* Wenn die Einheit handelt oder zaubert, muss der Default-Befehl gelöscht werden. */
        set_order(&u->thisorder, NULL);
    }
}

static int use_item(unit * u, const item_type * itype, int amount, struct order *ord)
{
    int i;

    i = get_pooled(u, itype->rtype, GET_DEFAULT, amount);
    if (amount > i) {
        /* TODO: message? eg. "not enough %, using only %" */
        amount = i;
    }
    if (amount == 0) {
        return ENOITEM;
    }

    if (itype->flags & ITF_CANUSE) {
        int result = callbacks.use_item(u, itype, amount, ord);
        if (result > 0) {
            use_pooled(u, itype->rtype, GET_DEFAULT, result);
        }
        return result;
    }
    return EUNUSABLE;
}

void monthly_healing(void)
{
    region *r;

    for (r = regions; r; r = r->next) {
        unit *u;
        double healingcurse = 0;

        if (r->attribs) {
            /* bonus zurücksetzen */
            curse *c = get_curse(r->attribs, &ct_healing);
            if (c != NULL) {
                healingcurse = curse_geteffect(c);
            }
        }
        for (u = r->units; u; u = u->next) {
            int umhp = unit_max_hp(u) * u->number;
            double p = 1.0;

            /* hp über Maximum bauen sich ab. Wird zb durch Elixier der Macht
             * oder verändertes Ausdauertalent verursacht */
            if (u->hp > umhp) {
                u->hp -= (int)ceil((u->hp - umhp) / 2.0);
                if (u->hp < umhp)
                    u->hp = umhp;
                continue;
            }

            if (u_race(u)->flags & RCF_NOHEAL)
                continue;
            if (fval(u, UFL_HUNGER))
                continue;

            if (fval(r->terrain, SEA_REGION) && u->ship == NULL
                && !(canswim(u) || canfly(u))) {
                continue;
            }

            p *= u_heal_factor(u);
            if (u->hp < umhp) {
                double maxheal = fmax(u->number, umhp / 20.0);
                int addhp;
                if (active_building(u, bt_find("inn"))) {
                    p *= 1.5;
                }
                /* pro punkt 5% höher */
                p *= (1.0 + healingcurse * 0.05);

                maxheal = p * maxheal;
                addhp = (int)maxheal;
                maxheal -= addhp;
                if (maxheal > 0.0 && chance(maxheal))
                    ++addhp;

                /* Aufaddieren der geheilten HP. */
                if (umhp > u->hp + addhp) umhp = u->hp + addhp;
                u->hp = umhp;

                /* soll man an negativer regeneration sterben können? */
                assert(u->hp > 0);
            }
        }
    }
}

static void remove_exclusive(order ** ordp)
{
    while (*ordp) {
        order *ord = *ordp;
        if (is_exclusive(ord)) {
            *ordp = ord->next;
            ord->next = NULL;
            free_order(ord);
        }
        else {
            ordp = &ord->next;
        }
    }
}

void defaultorders(void)
{
    region *r;

    assert(!keyword_disabled(K_DEFAULT));
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            bool neworders = false;
            order **ordp = &u->orders;
            while (*ordp != NULL) {
                order *ord = *ordp;
                if (getkeyword(ord) == K_DEFAULT) {
                    char lbuf[8192];
                    order *new_order = 0;
                    const char *s;
                    init_order_depr(ord);
                    s = gettoken(lbuf, sizeof(lbuf));
                    if (s) {
                        new_order = parse_order(s, u->faction->locale);
                    }
                    *ordp = ord->next;
                    ord->next = NULL;
                    free_order(ord);
                    if (!neworders) {
                        /* lange Befehle aus orders und old_orders löschen zu gunsten des neuen */
                        /* TODO: why only is_exclusive, not is_long? what about CAST, BUY, SELL? */
                        remove_exclusive(&u->orders);
                        remove_exclusive(&u->old_orders);
                        neworders = true;
                        ordp = &u->orders;  /* we could have broken ordp */
                    }
                    if (new_order)
                        addlist(&u->old_orders, new_order);
                }
                else
                    ordp = &ord->next;
            }
        }
    }
}

/* ************************************************************ */
/* GANZ WICHTIG! ALLE GEAENDERTEN SPRUECHE NEU ANZEIGEN */
/* GANZ WICHTIG! FUEGT AUCH NEUE ZAUBER IN DIE LISTE DER BEKANNTEN EIN */
/* ************************************************************ */
#define COMMONSPELLS 1          /* number of new common spells per level */
#define MAXMAGES 128            /* should be enough */

static int faction_getmages(faction * f, unit ** results, int numresults)
{
    unit *u;
    int maxlevel = 0, n = 0;

    for (u = f->units; u; u = u->nextF) {
        if (u->number > 0) {
            sc_mage *mage = get_mage_depr(u);
            if (mage) {
                int level = effskill(u, SK_MAGIC, 0);
                if (level > maxlevel) {
                    maxlevel = level;
                }
                if (n < numresults) {
                    results[n++] = u;
                }
            }
        }
    }
    if (n < numresults) {
        results[n] = 0;
    }
    return maxlevel;
}

static void copy_spells(const spellbook * src, spellbook * dst, int maxlevel)
{
    assert(dst);
    if (src && src->spells) {
        selist *ql;
        int qi;
        for (qi = 0, ql = src->spells; ql; selist_advance(&ql, &qi, 1)) {
            spellbook_entry * sbe = (spellbook_entry *)selist_get(ql, qi);
            if (sbe->level <= maxlevel) {
                spell *sp = spellref_get(&sbe->spref);
                if (!spellbook_get(dst, sp)) {
                    spellbook_add(dst, sp, sbe->level);
                }
            }
        }
    }
}

static void show_new_spells(faction * f, int level, const spellbook *book)
{
    if (book) {
        selist *ql = book->spells;
        int qi;

        for (qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            spellbook_entry *sbe = (spellbook_entry *)selist_get(ql, qi);
            if (sbe->level <= level) {
                show_spell(f, sbe);
            }
        }
    }
}

static void update_spells(void)
{
    faction *f;

    for (f = factions; f; f = f->next) {
        if (f->magiegebiet != M_NONE && !is_monsters(f)) {
            unit *mages[MAXMAGES];
            int i;
            int maxlevel = faction_getmages(f, mages, MAXMAGES);

            if (maxlevel && FactionSpells()) {
                spellbook * book = get_spellbook(magic_school[f->magiegebiet]);
                if (!f->spellbook) {
                    f->spellbook = create_spellbook(0);
                }
                copy_spells(book, f->spellbook, maxlevel);
                if (maxlevel > f->max_spelllevel) {
                    spellbook * common_spells = get_spellbook(magic_school[M_COMMON]);
                    pick_random_spells(f, maxlevel, common_spells, COMMONSPELLS);
                }
            }
            show_new_spells(f, maxlevel, faction_get_spellbook(f));
            for (i = 0; i != MAXMAGES && mages[i]; ++i) {
                unit * u = mages[i];
                sc_mage *mage = get_mage_depr(u);
                if (mage && mage->spellbook) {
                    int level = effskill(u, SK_MAGIC, 0);
                    show_new_spells(f, level, mage->spellbook);
                }
            }
        }
    }
}

int use_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *t;
    int n, err = ENOITEM;
    const item_type *itype;

    init_order_depr(ord);

    t = gettoken(token, sizeof(token));
    if (!t) {
        cmistake(u, ord, 43, MSG_PRODUCE);
        return err;
    }
    n = atoip((const char *)t);
    if (n == 0) {
        if (isparam(t, u->faction->locale, P_ANY)) {
            /* BENUTZE ALLES Yanxspirit */
            n = INT_MAX;
            t = gettoken(token, sizeof(token));
        }
        else {
            /* BENUTZE Yanxspirit */
            n = 1;
        }
    }
    else {
        /* BENUTZE 42 Yanxspirit */
        t = gettoken(token, sizeof(token));
    }
    itype = t ? finditemtype(t, u->faction->locale) : NULL;

    if (itype != NULL) {
        err = use_item(u, itype, n, ord);
    }
    switch (err) {
    case ENOITEM:
        cmistake(u, ord, 43, MSG_PRODUCE);
        break;
    case EUNUSABLE:
        cmistake(u, ord, 76, MSG_PRODUCE);
        break;
    case ENOSKILL:
        cmistake(u, ord, 50, MSG_PRODUCE);
        break;
    default:
        /* no error */
        break;
    }
    return err;
}

int pay_cmd(unit * u, struct order *ord)
{
    if (!u->building) {
        cmistake(u, ord, 6, MSG_EVENT);
    }
    else {
        building *b = NULL;
        param_t p;
        int id;

        init_order_depr(ord);
        p = getparam(u->faction->locale);
        id = getid();
        if (p == P_NOT) {
            unit *owner = building_owner(u->building);
            /* If the unit is not the owner of the building: error */
            if (owner->no != u->no) {
                /* The building is not ours error */
                cmistake(u, ord, 1222, MSG_EVENT);
            }
            else {
                /* If no building id is given or it is the id of our building, just set the do-not-pay flag */
                if (id == 0 || id == u->building->no)
                {
                    u->building->flags |= BLD_DONTPAY;
                }
                else {
                    /* Find the building that matches to the given id*/
                    b = findbuilding(id);
                    /* If there is a building and it is in the same region as the unit continue, else: error */
                    if (b && b->region == u->region)
                    {
                        /* If the unit is also the building owner (this is true if rules.region_owner_pay_building */
                        /* is set and no one is in the building) set the do-not-pay flag for this building, else: error */
                        if (building_owner(b) == u) {
                            b->flags |= BLD_DONTPAY;
                        }
                        else
                        {
                            /* The building is not ours error */
                            cmistake(u, ord, 1222, MSG_EVENT);
                        }

                    }
                    else
                    {
                        /* Building not found error */
                        cmistake(u, ord, 6, MSG_PRODUCE);
                    }
                }
            }
        }
    }
    return 0;
}

static int reserve_i(unit * u, struct order *ord, int flags)
{
    char token[128];
    if (u->number > 0 && (u_race(u)->ec_flags & ECF_GETITEM)) {
        int use, count, para;
        const item_type *itype;
        const char *s;

        init_order_depr(ord);
        s = gettoken(token, sizeof(token));
        count = s ? atoip(s) : 0;
        para = findparam(s, u->faction->locale);

        if (count == 0 && para == P_EACH) {
            count = getint() * u->number;
        }
        s = gettoken(token, sizeof(token));
        itype = s ? finditemtype(s, u->faction->locale) : 0;
        if (itype == NULL)
            return 0;

        set_resvalue(u, itype, 0);      /* make sure the pool is empty */

        if (count == 0 && para == P_ANY) {
            count = get_resource(u, itype->rtype);
        }
        use = use_pooled(u, itype->rtype, flags, count);
        if (use) {
            set_resvalue(u, itype, use);
            change_resource(u, itype->rtype, use);
            return use;
        }
    }
    return 0;
}

int reserve_cmd(unit * u, struct order *ord) {
    return reserve_i(u, ord, GET_DEFAULT);
}

int reserve_self(unit * u, struct order *ord) {
    return reserve_i(u, ord, GET_RESERVE | GET_SLACK);
}

int claim_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *t;
    int n = 1;
    const item_type *itype = 0;

    init_order_depr(ord);

    t = gettoken(token, sizeof(token));
    if (t) {
        n = atoip((const char *)t);
        if (n == 0) {
            n = 1;
        }
        else {
            t = gettoken(token, sizeof(token));
        }
        if (t) {
            itype = finditemtype(t, u->faction->locale);
        }
    }
    if (itype) {
        item **iclaim = i_find(&u->faction->items, itype);
        if (iclaim && *iclaim) {
            if (n > (*iclaim)->number) n = (*iclaim)->number;
            i_change(iclaim, itype, -n);
            i_change(&u->items, itype, n);
        }
    }
    else {
        cmistake(u, ord, 43, MSG_PRODUCE);
    }
    return 0;
}

enum {
    PROC_THISORDER = 1 << 0,
    PROC_LONGORDER = 1 << 1
};

typedef enum processor_t { PR_GLOBAL, PR_REGION_PRE, PR_UNIT, PR_ORDER, PR_REGION_POST } processor_t;

typedef struct processor {
    struct processor *next;
    int priority;
    processor_t type;
    unsigned int flags;
    union {
        struct {
            keyword_t kword;
            int(*process) (struct unit *, struct order *);
        } per_order;
        struct {
            void(*process) (struct unit *);
        } per_unit;
        struct {
            void(*process) (struct region *);
        } per_region;
        struct {
            void(*process) (void);
        } global;
    } data;
    const char *name;
} processor;

static processor *processors;

static processor *add_proc(int priority, const char *name, processor_t type)
{
    processor **pproc = &processors;
    processor *proc;

    while (*pproc) {
        proc = *pproc;
        if (proc->priority > priority)
            break;
        else if (proc->priority == priority && proc->type >= type)
            break;
        pproc = &proc->next;
    }

    proc = (processor *)malloc(sizeof(processor));
    proc->priority = priority;
    proc->type = type;
    proc->name = name;
    proc->next = *pproc;
    *pproc = proc;
    return proc;
}

void
add_proc_order(int priority, keyword_t kword, int(*parser) (struct unit *,
struct order *), unsigned int flags, const char *name)
{
    if (!keyword_disabled(kword)) {
        processor *proc = add_proc(priority, name, PR_ORDER);
        if (proc) {
            proc->data.per_order.process = parser;
            proc->data.per_order.kword = kword;
            proc->flags = flags;
        }
    }
}

void add_proc_global(int priority, void(*process) (void), const char *name)
{
    processor *proc = add_proc(priority, name, PR_GLOBAL);
    if (proc) {
        proc->data.global.process = process;
    }
}

void add_proc_region(int priority, void(*process) (region *), const char *name)
{
    processor *proc = add_proc(priority, name, PR_REGION_PRE);
    if (proc) {
        proc->data.per_region.process = process;
    }
}

void
add_proc_postregion(int priority, void(*process) (region *), const char *name)
{
    processor *proc = add_proc(priority, name, PR_REGION_POST);
    if (proc) {
        proc->data.per_region.process = process;
    }
}

void add_proc_unit(int priority, void(*process) (unit *), const char *name)
{
    processor *proc = add_proc(priority, name, PR_UNIT);
    if (proc) {
        proc->data.per_unit.process = process;
    }
}

/* per priority, execute processors in order from PR_GLOBAL down to PR_ORDER */
void process(void)
{
    processor *proc = processors;
    faction *f;

    while (proc) {
        int prio = proc->priority;
        region *r;
        processor *pglobal = proc;

        log_debug("- Step %u", prio);
        while (proc && proc->priority == prio) {
            if (proc->name)
                log_debug(" - %s", proc->name);
            proc = proc->next;
        }

        while (pglobal && pglobal->priority == prio && pglobal->type == PR_GLOBAL) {
            pglobal->data.global.process();
            pglobal = pglobal->next;
        }
        if (pglobal == NULL || pglobal->priority != prio)
            continue;

        for (r = regions; r; r = r->next) {
            unit *u;
            processor *pregion = pglobal;

            while (pregion && pregion->priority == prio
                && pregion->type == PR_REGION_PRE) {
                pregion->data.per_region.process(r);
                pregion = pregion->next;
            }
            if (pregion == NULL || pregion->priority != prio)
                continue;

            if (r->units) {
                for (u = r->units; u; u = u->next) {
                    processor *porder, *punit = pregion;

                    while (punit && punit->priority == prio && punit->type == PR_UNIT) {
                        punit->data.per_unit.process(u);
                        punit = punit->next;
                    }
                    if (punit == NULL || punit->priority != prio)
                        continue;

                    porder = punit;
                    while (porder && porder->priority == prio && porder->type == PR_ORDER) {
                        order **ordp = &u->orders;
                        if (porder->flags & PROC_THISORDER)
                            ordp = &u->thisorder;
                        while (*ordp) {
                            order *ord = *ordp;
                            if (getkeyword(ord) == porder->data.per_order.kword) {
                                if (porder->flags & PROC_LONGORDER) {
                                    if (u->number == 0) {
                                        ord = NULL;
                                    }
                                    else if (u_race(u) == get_race(RC_INSECT)
                                        && r_insectstalled(r)
                                        && !is_cursed(u->attribs, &ct_insectfur)) {
                                        ord = NULL;
                                    }
                                    else if (LongHunger(u)) {
                                        cmistake(u, ord, 224, MSG_MAGIC);
                                        ord = NULL;
                                    }
                                    else if (fval(u, UFL_LONGACTION)) {
                                        /* this message was already given in laws.update_long_order
                                           cmistake(u, ord, 52, MSG_PRODUCE);
                                           */
                                        ord = NULL;
                                    }
                                    else if (fval(r->terrain, SEA_REGION)
                                        && u_race(u) != get_race(RC_AQUARIAN)
                                        && !(u_race(u)->flags & RCF_SWIM)) {
                                        /* error message disabled by popular demand */
                                        ord = NULL;
                                    }
                                }
                                if (ord) {
                                    porder->data.per_order.process(u, ord);
                                }
                            }
                            if (!ord || *ordp == ord)
                                ordp = &(*ordp)->next;
                        }
                        porder = porder->next;
                    }
                }
            }

            while (pregion && pregion->priority == prio
                && pregion->type != PR_REGION_POST) {
                pregion = pregion->next;
            }

            while (pregion && pregion->priority == prio
                && pregion->type == PR_REGION_POST) {
                pregion->data.per_region.process(r);
                pregion = pregion->next;
            }
            if (pregion == NULL || pregion->priority != prio)
                continue;

        }
    }

    log_debug("\n - Leere Gruppen loeschen...\n");
    for (f = factions; f; f = f->next) {
        group **gp = &f->groups;
        while (*gp) {
            group *g = *gp;
            if (g->members == 0) {
                *gp = g->next;
                free_group(g);
            }
            else
                gp = &g->next;
        }
    }

}

int armedmen(const unit * u, bool siege_weapons)
{
    item *itm;
    int n = 0;
    if (!(u_race(u)->flags & RCF_NOWEAPONS)) {
        if (effskill(u, SK_WEAPONLESS, 0) >= 1) {
            /* kann ohne waffen bewachen: fuer drachen */
            n = u->number;
        }
        else {
            /* alle Waffen werden gezaehlt, und dann wird auf die Anzahl
            * Personen minimiert */
            for (itm = u->items; itm; itm = itm->next) {
                const weapon_type *wtype = resource2weapon(itm->type->rtype);
                if (wtype == NULL || (!siege_weapons && (wtype->flags & WTF_SIEGE)))
                    continue;
                if (effskill(u, wtype->skill, 0) >= 1)
                    n += itm->number;
                if (n >= u->number)
                    break;
            }
            if (n > u->number) n = u->number;
        }
    }
    return n;
}

int siege_cmd(unit * u, order * ord)
{
    region *r = u->region;
    building *b;
    int d, pooled;
    int bewaffnete, katapultiere = 0;
    resource_type *rt_catapultammo = NULL;
    resource_type *rt_catapult = NULL;

    init_order_depr(ord);
    b = getbuilding(r);

    if (!b) {
        cmistake(u, ord, 31, MSG_BATTLE);
        return 31;
    }

    if (!playerrace(u_race(u))) {
        /* keine Drachen, Illusionen, Untote etc */
        cmistake(u, ord, 166, MSG_BATTLE);
        return 166;
    }
    /* schaden durch katapulte */

    rt_catapultammo = rt_find("catapultammo");
    rt_catapult = rt_find("catapult");

    d = i_get(u->items, rt_catapult->itype);
    if (d > u->number) d = u->number;
    pooled = get_pooled(u, rt_catapultammo, GET_DEFAULT, d);
    if (d > pooled) d = pooled;
    if (effskill(u, SK_CATAPULT, 0) >= 1) {
        katapultiere = d;
        d *= effskill(u, SK_CATAPULT, 0);
    }
    else {
        d = 0;
    }

    bewaffnete = armedmen(u, true);
    if (d == 0 && bewaffnete == 0) {
        /* abbruch, falls unbewaffnet oder unfaehig, katapulte zu benutzen */
        cmistake(u, ord, 80, MSG_EVENT);
        return 80;
    }

    if (!is_guard(u)) {
        /* abbruch, wenn die einheit nicht vorher die region bewacht - als
         * warnung fuer alle anderen! */
        cmistake(u, ord, 81, MSG_EVENT);
        return 81;
    }
    /* einheit und burg markieren - spart zeit beim behandeln der einheiten
     * in der burg, falls die burg auch markiert ist und nicht alle
     * einheiten wieder abgesucht werden muessen! */

    usetsiege(u, b);
    if (katapultiere < bewaffnete) katapultiere = bewaffnete;
    b->besieged += katapultiere;

    /* definitiver schaden eingeschraenkt */
    if (d > b->size - 1) d = b->size - 1;

    /* meldung, schaden anrichten */
    if (d && !curse_active(get_curse(b->attribs, &ct_magicwalls))) {
        b->size -= d;
        use_pooled(u, rt_catapultammo,
            GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, d);
        /* send message to the entire region */
        ADDMSG(&r->msgs, msg_message("siege_catapults",
            "unit building destruction", u, b, d));
    }
    else {
        /* send message to the entire region */
        ADDMSG(&r->msgs, msg_message("siege", "unit building", u, b));
    }
    return 0;
}

void do_siege(region * r)
{
    if (fval(r->terrain, LAND_REGION)) {
        unit *u;

        for (u = r->units; u; u = u->next) {
            if (getkeyword(u->thisorder) == K_BESIEGE) {
                siege_cmd(u, u->thisorder);
            }
        }
    }
}

static void enter_1(region * r)
{
    do_enter(r, 0);
}

static void enter_2(region * r)
{
    do_enter(r, 1);
}

bool help_enter(unit *uo, unit *u) {
    return uo->faction == u->faction || alliedunit(uo, u->faction, HELP_GUARD);
}

static void do_force_leave(region *r) {
    force_leave(r, NULL);
}

bool rule_force_leave(int flags) {
    int rules = config_get_int("rules.owners.force_leave", 0);
    return (rules&flags) == flags;
}

void init_processor(void)
{
    int p;

    while (processors) {
        processor * next = processors->next;
        free(processors);
        processors = next;
    }

    p = 10;
    add_proc_global(p, nmr_warnings, "NMR Warnings");
    add_proc_global(p, new_units, "Neue Einheiten erschaffen");

    p += 10;
    add_proc_unit(p, update_long_order, "Langen Befehl aktualisieren");
    add_proc_order(p, K_BANNER, banner_cmd, 0, NULL);
    add_proc_order(p, K_EMAIL, email_cmd, 0, NULL);
    add_proc_order(p, K_PASSWORD, password_cmd, 0, NULL);
    add_proc_order(p, K_SEND, send_cmd, 0, NULL);
    add_proc_order(p, K_GROUP, group_cmd, 0, NULL);

    p += 10;
    add_proc_order(p, K_QUIT, quit_cmd, 0, NULL);
    add_proc_order(p, K_URSPRUNG, origin_cmd, 0, NULL);
    add_proc_order(p, K_ALLY, ally_cmd, 0, NULL);
    add_proc_order(p, K_PREFIX, prefix_cmd, 0, NULL);
    add_proc_order(p, K_SETSTEALTH, setstealth_cmd, 0, NULL);
    add_proc_order(p, K_STATUS, status_cmd, 0, NULL);
    add_proc_order(p, K_COMBATSPELL, combatspell_cmd, 0, NULL);
    add_proc_order(p, K_DISPLAY, display_cmd, 0, NULL);
    add_proc_order(p, K_NAME, name_cmd, 0, NULL);
    add_proc_order(p, K_GUARD, guard_off_cmd, 0, NULL);
    add_proc_order(p, K_RESHOW, reshow_cmd, 0, NULL);

    if (config_get_int("rules.alliances", 0) == 1) {
        p += 10;
        add_proc_global(p, alliance_cmd, NULL);
    }

    p += 10;
    add_proc_region(p, do_contact, "Kontaktieren");
    add_proc_order(p, K_MAIL, mail_cmd, 0, "Botschaften");

    p += 10;                      /* all claims must be done before we can USE */
    add_proc_region(p, enter_1, "Betreten (1. Versuch)");     /* for GIVE CONTROL */
    add_proc_order(p, K_USE, use_cmd, 0, "Benutzen");

    p += 10;                      /* in case it has any effects on alliance victories */
    add_proc_order(p, K_GIVE, give_control_cmd, 0, "GIB KOMMANDO");

    p += 10;                      /* in case it has any effects on alliance victories */
    add_proc_order(p, K_LEAVE, leave_cmd, 0, "Verlassen");

    p += 10;
    add_proc_region(p, enter_1, "Betreten (2. Versuch)"); /* to allow a buildingowner to enter the castle pre combat */

    p += 10;
    add_proc_global(p, do_battles, "Attackieren");

    if (!keyword_disabled(K_BESIEGE)) {
        p += 10;
        add_proc_region(p, do_siege, "Belagern");
    }

    p += 10;                      /* can't allow reserve before siege (weapons) */
    add_proc_region(p, enter_1, "Betreten (3. Versuch)");  /* to claim a castle after a victory and to be able to DESTROY it in the same turn */
    if (config_get_int("rules.reserve.twophase", 0)) {
        add_proc_order(p, K_RESERVE, reserve_self, 0, "RESERVE (self)");
        p += 10;
    }
    add_proc_order(p, K_RESERVE, reserve_cmd, 0, "RESERVE (all)");
    add_proc_order(p, K_CLAIM, claim_cmd, 0, NULL);
    add_proc_unit(p, follow_unit, "Folge auf Einheiten setzen");

    p += 10;                      /* rest rng again before economics */
    if (rule_force_leave(FORCE_LEAVE_ALL)) {
        add_proc_region(p, do_force_leave, "kick non-allies out of buildings/ships");
    }
    add_proc_region(p, economics, "Zerstoeren, Geben, Rekrutieren, Vergessen");
    add_proc_order(p, K_PROMOTION, promotion_cmd, 0, "Heldenbefoerderung");

    p += 10;
    if (!keyword_disabled(K_PAY)) {
        add_proc_order(p, K_PAY, pay_cmd, 0, "Gebaeudeunterhalt (BEZAHLE NICHT)");
    }
    add_proc_postregion(p, maintain_buildings, "Gebaeudeunterhalt");

    p += 10;                      /* QUIT fuer sich alleine */
    add_proc_global(p, quit, "Sterben");

    if (!keyword_disabled(K_CAST)) {
        p += 10;
        add_proc_global(p, magic, "Zaubern");
    }

    p += 10;
    add_proc_order(p, K_TEACH, teach_cmd, PROC_THISORDER | PROC_LONGORDER,
        "Lehren");
    p += 10;
    add_proc_order(p, K_STUDY, study_cmd, PROC_THISORDER | PROC_LONGORDER,
        "Lernen");

    p += 10;
    add_proc_order(p, K_MAKE, make_cmd, PROC_THISORDER | PROC_LONGORDER,
        "Produktion");
    add_proc_postregion(p, produce, "Arbeiten, Handel, Rekruten");
    add_proc_postregion(p, split_allocations, "Produktion II");

    p += 10;
    add_proc_region(p, enter_2, "Betreten (4. Versuch)"); /* Once again after QUIT */

    p += 10;
    add_proc_region(p, sinkships, "Schiffe sinken");

    p += 10;
    add_proc_global(p, movement, "Bewegungen");

    if (config_get_int("work.auto", 0)) {
        p += 10;
        add_proc_region(p, auto_work, "Arbeiten (auto)");
    }

    p += 10;
    add_proc_order(p, K_GUARD, guard_on_cmd, 0, "Bewache (an)");

    p += 10;
    add_proc_unit(p, monster_kills_peasants,
        "Monster fressen und vertreiben Bauern");

    p += 10;
    add_proc_global(p, randomevents, "Zufallsereignisse");

    p += 10;

    add_proc_global(p, monthly_healing, "Regeneration (HP)");
    add_proc_global(p, regenerate_aura, "Regeneration (Aura)");
    if (!keyword_disabled(K_DEFAULT)) {
        add_proc_global(p, defaultorders, "Defaults setzen");
    }
    add_proc_global(p, demographics, "Nahrung, Seuchen, Wachstum, Wanderung");

    if (!keyword_disabled(K_SORT)) {
        p += 10;
        add_proc_global(p, restack_units, "Einheiten sortieren");
    }
    if (!keyword_disabled(K_NUMBER)) {
        add_proc_order(p, K_NUMBER, renumber_cmd, 0, "Neue Nummern (Einheiten)");
        p += 10;
        add_proc_global(p, renumber_factions, "Neue Nummern");
    }
}

static void reset_game(void)
{
    region *r;
    faction *f;
    for (r = regions; r; r = r->next) {
        unit *u;
        building *b;
        r->flags &= RF_SAVEMASK;
        for (u = r->units; u; u = u->next) {
            u->flags &= UFL_SAVEMASK;
        }
        for (b = r->buildings; b; b = b->next) {
            b->flags &= BLD_SAVEMASK;
        }
        if (r->land && r->land->ownership && r->land->ownership->owner) {
            faction *owner = r->land->ownership->owner;
            if (owner == get_monsters()) {
                /* some compat-fix, i believe. */
                owner = update_owners(r);
            }
            if (owner) {
                fset(r, RF_GUARDED);
            }
        }
    }
    for (f = factions; f; f = f->next) {
        f->flags &= FFL_SAVEMASK;
    }
}

void turn_begin(void)
{
    int handle_start = first_turn();
    assert(turn >= 0);
    if (turn < handle_start) {
        /* this should only happen during tests */
        turn = handle_start;
    }
    ++turn;
    reset_game();
}

void turn_process(void)
{
    init_processor();
    process();

    if (markets_module()) {
        do_markets();
    }
}

void turn_end(void)
{
    log_info(" - Attribute altern");
    ageing();
    remove_empty_units();

    /* must happen AFTER age, because that would destroy them right away */
    if (config_get_int("modules.wormhole", 0)) {
        wormholes_update();
    }

    /* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
     * Beschreibungen geändert haben */
    update_spells();
}

void update_subscriptions(void)
{
    FILE *F;
    char zText[4096];

    path_join(basepath(), "subscriptions", zText, sizeof(zText));
    F = fopen(zText, "r");
    if (F == NULL) {
        log_warning(0, "could not open %s.\n", zText);
        return;
    }
    for (;;) {
        char zFaction[5];
        int subscription, fno;
        faction *f;

        if (fscanf(F, "%4d %4s", &subscription, zFaction) <= 0)
            break;
        fno = atoi36(zFaction);
        f = findfaction(fno);
        if (f != NULL) {
            f->subscription = subscription;
        }
    }
    fclose(F);
}

/** determine if unit can be seen by faction
 * @param f -- the observiong faction
 * @param u -- the unit that is observed
 * @param r -- the region that u is obesrved in (see below)
 * @param m -- terrain modifier to stealth
 * 
 * r kann != u->region sein, wenn es um Durchreisen geht,
 * oder Zauber (sp_generous, sp_fetchastral).
 * Es muss auch niemand aus f in der region sein, wenn sie vom Turm
 * erblickt wird */
bool
cansee(const faction * f, const region * r, const unit * u, int modifier)
{
    int stealth, rings;

    if (u->faction == f || omniscient(f)) {
        return true;
    }
    else if (fval(u_race(u), RCF_INVISIBLE)) {
        return false;
    }
    else if (u->number == 0) {
        attrib *a = a_find(u->attribs, &at_creator);
        if (a) {                    /* u is an empty temporary unit. In this special case
                                    we look at the creating unit. */
            u = (unit *)a->data.v;
        }
        else {
            return false;
        }
    }

    /* simple visibility, just gotta have a viewer in the region to see 'em */
    if (leftship(u) || is_guard(u) || usiege(u) || u->building || u->ship) {
        return true;
    }

    rings = invisible(u, NULL);
    stealth = eff_stealth(u, r) - modifier;

    unit *u2;
    for (u2 = r->units; u2; u2 = u2->next) {
        if (u2->faction == f) {
            if (rings < u->number || invisible(u, u2) < u->number) {
                if (skill_enabled(SK_PERCEPTION)) {
                    int observation = effskill(u2, SK_PERCEPTION, 0);

                    if (observation >= stealth) {
                        return true;
                    }
                }
                else {
                    return true;
                }
            }
        }
    }

    return (rings <= 0 && stealth <= 0);
}

bool cansee_unit(const unit * u, const unit * target, int modifier)
/* target->region kann != u->region sein, wenn es um durchreisen geht */
{
    if (fval(u_race(target), RCF_INVISIBLE) || target->number == 0)
        return false;
    else if (target->faction == u->faction)
        return true;
    else {
        int n, rings;

        if (is_guard(target) || usiege(target) || target->building
            || target->ship) {
            return true;
        }

        n = eff_stealth(target, target->region) - modifier;
        rings = invisible(target, NULL);
        if (rings == 0 && n <= 0) {
            return true;
        }

        if (rings && invisible(target, u) >= target->number) {
            return false;
        }
        if (skill_enabled(SK_PERCEPTION)) {
            int o = effskill(u, SK_PERCEPTION, target->region);
            if (o >= n) {
                return true;
            }
        }
        else {
            return true;
        }
    }
    return false;
}

bool
cansee_durchgezogen(const faction * f, const region * r, const unit * u,
    int modifier)
    /* r kann != u->region sein, wenn es um durchreisen geht */
    /* und es muss niemand aus f in der region sein, wenn sie vom Turm
    * erblickt wird */
{
    unit *u2;

    if (fval(u_race(u), RCF_INVISIBLE) || u->number == 0)
        return false;
    else if (u->faction == f)
        return true;
    else {
        int rings, n;

        if (is_guard(u) || usiege(u) || u->building || u->ship) {
            return true;
        }

        n = eff_stealth(u, r) - modifier;
        rings = invisible(u, NULL);
        if (rings == 0 && n <= 0) {
            return true;
        }

        for (u2 = r->units; u2; u2 = u2->next) {
            if (u2->faction == f) {
                int o;

                if (rings && invisible(u, u2) >= u->number)
                    continue;

                o = effskill(u2, SK_PERCEPTION, 0);

                if (o >= n) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool
seefaction(const faction * f, const region * r, const unit * u, int modifier)
{
    if (((f == u->faction) || !fval(u, UFL_ANON_FACTION))
        && cansee(f, r, u, modifier))
        return true;
    return false;
}
