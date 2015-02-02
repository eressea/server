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
#include "laws.h"

#include <modules/gmcmd.h>

#include "alchemy.h"
#include "battle.h"
#include "economy.h"
#include "keyword.h"
#include "market.h"
#include "morale.h"
#include "monster.h"
#include "move.h"
#include "randenc.h"
#include "spy.h"
#include "study.h"
#include "wormhole.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/ally.h>
#include <kernel/connection.h>
#include <kernel/curse.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>   /* for volcanoes in emigration (needs a flag) */
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/stealth.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <quicklist.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/umlaut.h>
#include <util/message.h>
#include <util/rng.h>

#include <attributes/otherfaction.h>

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

#include <tests.h>

/* chance that a peasant dies of starvation: */
#define PEASANT_STARVATION_CHANCE 0.9F
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

static int RemoveNMRNewbie(void)
{
    static int value = -1;
    static int gamecookie = -1;

    if (value < 0 || gamecookie != global.cookie) {
        value = get_param_int(global.parameters, "nmr.removenewbie", 0);
        gamecookie = global.cookie;
    }
    return value;
}

static void checkorders(void)
{
    faction *f;

    log_info(" - Warne spaete Spieler...");
    for (f = factions; f; f = f->next)
        if (!is_monsters(f) && turn - f->lastorders == NMRTimeout() - 1)
            ADDMSG(&f->msgs, msg_message("turnreminder", ""));
}

static void age_unit(region * r, unit * u)
{
    if (u_race(u) == get_race(RC_SPELL)) {
        if (--u->age <= 0) {
            remove_unit(&r->units, u);
        }
    }
    else {
        ++u->age;
        if (u->number > 0 && u_race(u)->age) {
            u_race(u)->age(u);
        }
    }
    if (u->region && is_astral(u->region)) {
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
                int weeks = _min(effect, u->number);
                reduce_skill(u, sb, weeks);
                ADDMSG(&u->faction->msgs, msg_message("dumbeffect",
                    "unit weeks skill", u, weeks, (skill_t)sb->id));
            }                         /* sonst Glück gehabt: wer nix weiß, kann nix vergessen... */
            change_effect(u, oldpotiontype[P_FOOL], -effect);
        }
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
    int maxp = maxworkingpeasants(r);
    int rp = rpeasants(r);
    int max_immigrants = MAX_IMMIGRATION(maxp - rp);

    if (r->terrain == newterrain(T_VOLCANO)
        || r->terrain == newterrain(T_VOLCANO_SMOKING)) {
        max_immigrants = max_immigrants / 10;
    }

    for (i = 0; max_immigrants > 0 && i != MAXDIRECTIONS; i++) {
        int dir = (turn + i) % MAXDIRECTIONS;
        region *rc = rconnect(r, (direction_t)dir);

        if (rc != NULL && fval(rc->terrain, LAND_REGION)) {
            int rp2 = rpeasants(rc);
            int maxp2 = maxworkingpeasants(rc);
            int max_emigration = MAX_EMIGRATION(rp2 - maxp2);

            if (max_emigration > 0) {
                max_emigration = _min(max_emigration, max_immigrants);
                r->land->newpeasants += max_emigration;
                rc->land->newpeasants -= max_emigration;
                max_immigrants -= max_emigration;
            }
        }
    }
}


static float peasant_growth_factor(void)
{
    return get_param_flt(global.parameters, "rules.peasants.growth.factor", 0.0001F * PEASANTGROWTH);
}

#ifdef SLOWLUCK
int peasant_luck_effect(int peasants, int luck, int maxp, float variance) {
    int n, births=0;
    float factor = peasant_growth_factor();
    for (n = peasants; n && luck; --n) {
        int chances = 0;

        if (luck > 0) {
            --luck;
            chances += PEASANTLUCK;
        }

        while (chances--) {
            if (rng_double() < factor) {
                /* Only raise with 75% chance if peasants have
                * reached 90% of maxpopulation */
                if (peasants / (float)maxp < 0.9 || chance(PEASANTFORCE)) {
                    ++births;
                }
            }
        }
    }
    return births;
}
#else
static float peasant_luck_factor(void)
{
    return get_param_flt(global.parameters, "rules.peasants.peasantluck.factor", PEASANTLUCK);
}

int peasant_luck_effect(int peasants, int luck, int maxp, float variance)
{
    int births = 0;
    double mean = _min(luck, peasants) * peasant_luck_factor()
        * peasant_growth_factor() * ((peasants / (float)maxp < .9) ? 1 :
        PEASANTFORCE);

    births = RAND_ROUND(normalvariate(mean, variance * mean));
    if (births <= 0)
        births = 1;
    if (births > peasants / 2)
        births = peasants / 2 + 1;
    return births;
}

#endif

static void peasants(region * r)
{
    int peasants = rpeasants(r);
    int money = rmoney(r);
    int maxp = production(r);
    int n, satiated;
    int dead = 0;

    if (peasants > 0 && get_param_int(global.parameters, "rules.peasants.growth", 1)) {
        int luck = 0;
        double fraction = peasants * peasant_growth_factor();
        int births = RAND_ROUND(fraction);
        attrib *a = a_find(r->attribs, &at_peasantluck);

        if (a != NULL) {
            luck = a->data.i * 1000;
        }

        luck = peasant_luck_effect(peasants, luck, maxp, .5);
#ifdef STORCH_SPAM_BUG_2072
        ADDMSG(&r->msgs, msg_message("peasantluck_success", "births", luck));
#endif
        peasants += births + luck;
    }

    /* Alle werden satt, oder halt soviele für die es auch Geld gibt */

    satiated = _min(peasants, money / maintenance_cost(NULL));
    rsetmoney(r, money - satiated * maintenance_cost(NULL));

    /* Von denjenigen, die nicht satt geworden sind, verhungert der
     * Großteil. dead kann nie größer als rpeasants(r) - satiated werden,
     * so dass rpeasants(r) >= 0 bleiben muß. */

    /* Es verhungert maximal die unterernährten Bevölkerung. */

    n = _min(peasants - satiated, rpeasants(r));
    dead += (int)(0.5F + n * PEASANT_STARVATION_CHANCE);

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
         * ich habe es wieder aktiviert, muß getestet werden.
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
    maxhorses = maxworkingpeasants(r) / 10;
    maxhorses = _max(0, maxhorses);
    horses = rhorses(r);
    if (horses > 0) {
        if (is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
            rsethorses(r, (int)(horses * 0.9F));
        }
        else if (maxhorses) {
            int i;
            double growth =
                (RESOURCE_QUANTITY * HORSEGROWTH * 200 * (maxhorses -
                horses)) / maxhorses;

            if (growth > 0) {
                if (a_find(r->attribs, &at_horseluck))
                    growth *= 2;
                /* printf("Horses: <%d> %d -> ", growth, horses); */
                i = (int)(0.5F + (horses * 0.0001F) * growth);
                /* printf("%d\n", horses); */
                rsethorses(r, horses + i);
            }
        }
    }

    /* Pferde wandern in Nachbarregionen.
     * Falls die Nachbarregion noch berechnet
     * werden muß, wird eine migration-Struktur gebildet,
     * die dann erst in die Berechnung der Nachbarstruktur einfließt.
     */

    for (n = 0; n != MAXDIRECTIONS; n++) {
        region *r2 = rconnect(r, n);
        if (r2 && fval(r2->terrain, WALK_INTO)) {
            int pt = (rhorses(r) * HORSEMOVE) / 100;
            pt = (int)normalvariate(pt, pt / 4.0);
            pt = _max(0, pt);
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

extern struct attrib_type at_germs;

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
    int growth, grownup_trees, i, seeds, sprout;
    direction_t d;
    attrib *a;

    if (current_season == SEASON_SUMMER || current_season == SEASON_AUTUMN) {
        double seedchance = 0.01F * RESOURCE_QUANTITY;
        int elves = count_race(r, get_race(RC_ELF));

        a = a_find(r->attribs, &at_germs);
        if (a && last_weeks_season == SEASON_SPRING) {
            /* ungekeimte Samen bleiben erhalten, Sprößlinge wachsen */
            sprout = _min(a->data.sa[1], rtrees(r, 1));
            /* aus dem gesamt Sprößlingepool abziehen */
            rsettrees(r, 1, rtrees(r, 1) - sprout);
            /* zu den Bäumen hinzufügen */
            rsettrees(r, 2, rtrees(r, 2) + sprout);

            a_removeall(&r->attribs, &at_germs);
        }

        if (is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
            rsettrees(r, 1, (int)(rtrees(r, 1) * 0.9));
            rsettrees(r, 2, (int)(rtrees(r, 2) * 0.9));
            return;
        }

        if (production(r) <= 0)
            return;

        /* Grundchance 1.0% */
        /* Jeder Elf in der Region erhöht die Chance marginal */
        elves = _min(elves, production(r) / 8);
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
                seedchance = (1000 * maxworkingpeasants(r2)) / r2->terrain->size;
                for (i = 0; i < seeds / MAXDIRECTIONS; i++) {
                    if (rng_int() % 10000 < seedchance)
                        sprout++;
                }
                rsettrees(r2, 0, rtrees(r2, 0) + sprout);
            }
        }

    }
    else if (current_season == SEASON_SPRING) {

        if (is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0))
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
        /* wir haben 6 Wochen zum wachsen, jeder Same/Sproß hat 18% Chance
         * zu wachsen, damit sollten nach 5-6 Wochen alle gewachsen sein */
        growth = 1800;

        /* Samenwachstum */

        /* Raubbau abfangen, es dürfen nie mehr Samen wachsen, als aktuell
         * in der Region sind */
        seeds = _min(a->data.sa[0], rtrees(r, 0));
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
        sprout = _min(a->data.sa[1], rtrees(r, 1));
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
    log_info(" - Einwanderung...");
    int repopulate = get_param_int(global.parameters, "rules.economy.repopulate_maximum", 90);
    for (r = regions; r; r = r->next) {
        if (r->land && r->land->newpeasants) {
            int rp = rpeasants(r) + r->land->newpeasants;
            rsetpeasants(r, _max(0, rp));
        }
        /* Genereate some (0-2 to 0-6 depending on the income) peasants out of nothing */
        /*if less then 50 are in the region and there is space and no monster or deamon units in the region */
        int peasants = rpeasants(r);
        if (repopulate && r->land && (peasants < repopulate) && maxworkingpeasants(r) >(peasants + 30) * 2)
        {
            int badunit = 0;
            unit *u;
            for (u = r->units; u; u = u->next) {
                if (!playerrace(u_race(u)) || u_race(u) == get_race(RC_DAEMON))
                {
                    badunit = 1;
                    break;
                }
            }
            if (badunit == 0)
            {
                peasants += (int)(rng_double()*(wage(r, NULL, NULL, turn) - 9));
                rsetpeasants(r, peasants);
            }
        }
    }
}

void demographics(void)
{
    region *r;
    static int last_weeks_season = -1;
    static int current_season = -1;

    if (current_season < 0) {
        gamedate date;
        get_gamedate(turn, &date);
        current_season = date.season;
        get_gamedate(turn - 1, &date);
        last_weeks_season = date.season;
    }

    for (r = regions; r; r = r->next) {
        ++r->age;                   /* also oceans. no idea why we didn't always do that */
        live(r);

        if (!fval(r->terrain, SEA_REGION)) {
            /* die Nachfrage nach Produkten steigt. */
            struct demand *dmd;
            if (r->land) {
                static int plant_rules = -1;

                if (plant_rules < 0) {
                    plant_rules =
                        get_param_int(global.parameters, "rules.economy.grow", 0);
                }
                for (dmd = r->land->demands; dmd; dmd = dmd->next) {
                    if (dmd->value > 0 && dmd->value < MAXDEMAND) {
                        float rise = DMRISE;
                        if (buildingtype_exists(r, bt_find("harbour"), true))
                            rise = DMRISEHAFEN;
                        if (rng_double() < rise)
                            ++dmd->value;
                    }
                }
                /* Seuchen erst nachdem die Bauern sich vermehrt haben
                 * und gewandert sind */

                calculate_emigration(r);
                peasants(r);
                if (r->age > 20) {
                    double mwp = _max(maxworkingpeasants(r), 1);
                    double prob =
                        pow(rpeasants(r) / (mwp * wage(r, NULL, NULL, turn) * 0.13), 4.0)
                        * PLAGUE_CHANCE;

                    if (rng_double() < prob) {
                        plagues(r);
                    }
                }
                horses(r);
                if (plant_rules == 0) { /* E1 */
                    growing_trees(r, current_season, last_weeks_season);
                    growing_herbs(r, current_season, last_weeks_season);
                }
                else {                /* E3 */
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
    checkorders();
}

/* ------------------------------------------------------------- */

static int modify(int i)
{
    int c;

    c = i * 2 / 3;

    if (c >= 1) {
        return (c + rng_int() % c);
    }
    else {
        return (i);
    }
}

static void inactivefaction(faction * f)
{
    FILE *inactiveFILE;
    char zText[128];

    sprintf(zText, "%s/%s", datapath(), "inactive");
    inactiveFILE = fopen(zText, "a");

    if (inactiveFILE) {
        fprintf(inactiveFILE, "%s:%s:%d:%d\n",
            factionid(f),
            LOC(default_locale, rc_name_s(f->race, NAME_PLURAL)),
            modify(count_all(f)), turn - f->lastorders);

        fclose(inactiveFILE);
    }
}

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
    n = eff_skill(u, SK_STEALTH, r);

    for (u2 = r->units; u2; u2 = u2->next) {
        if (usiege(u2) == b) {

            if (invisible(u, u2) >= u->number)
                continue;

            o = eff_skill(u2, SK_PERCEPTION, r);

            if (o + 2 >= n) {
                return 0;               /* entdeckt! */
            }
        }
    }
    return 1;
}

int can_contact(const region * r, const unit * u, const unit * u2) {

    /* hier geht es nur um die belagerung von burgen */

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

    init_order(ord);
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

    kwd = init_order(ord);
    assert(kwd == K_QUIT);
    passwd = gettoken(token, sizeof(token));
    if (checkpasswd(f, (const char *)passwd)) {
        fset(f, FFL_QUIT);
    }
    else {
        char buffer[64];
        write_order(ord, buffer, sizeof(buffer));
        cmistake(u, ord, 86, MSG_EVENT);
        log_warning("QUIT with illegal password for faction %s: %s\n", factionid(f), buffer);
    }
    return 0;
}

static bool mayenter(region * r, unit * u, building * b)
{
    unit *u2;
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
    static int value = -1;
    if (value < 0) {
        value = get_param_int(global.parameters, "rules.check_overload", 0);
    }
    return value != 0;
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

                init_order(ord);
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
    static int rule = -1;
    if (rule < 0)
        rule = get_param_int(global.parameters, "rules.nmr.destroy", 0);
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
    faction *f;

    log_info(" - beseitige Spieler, die sich zu lange nicht mehr gemeldet haben...");

    for (f = factions; f; f = f->next) {
        if (fval(f, FFL_NOIDLEOUT)) {
            f->lastorders = turn;
        }
        if (NMRTimeout() > 0 && turn - f->lastorders >= NMRTimeout()) {
            nmr_death(f);
            destroyfaction(f);
            continue;
        }
        if (turn != f->lastorders) {
            char info[256];
            sprintf(info, "%d Einheiten, %d Personen, %d Silber",
                f->no_units, f->num_total, f->money);
        }

        if (NMRTimeout() > 0 && turn - f->lastorders >= (NMRTimeout() - 1)) {
            inactivefaction(f);
            continue;
        }
    }
    log_info(" - beseitige Spieler, die sich nach der Anmeldung nicht gemeldet haben...");

    age = calloc(_max(4, turn + 1), sizeof(int));
    for (f = factions; f; f = f->next)
        if (!is_monsters(f)) {
            if (RemoveNMRNewbie() && !fval(f, FFL_NOIDLEOUT)) {
                if (f->age >= 0 && f->age <= turn)
                    ++age[f->age];
                if (f->age == 2 || f->age == 3) {
                    if (f->lastorders == turn - 2) {
                        destroyfaction(f);
                        ++dropouts[f->age - 2];
                        continue;
                    }
                }
            }
        }
}

void quit(void)
{
    faction **fptr = &factions;
    while (*fptr) {
        faction *f = *fptr;
        if (f->flags & FFL_QUIT) {
            destroyfaction(f);
        }
        else {
            ++f->age;
            if (f->age + 1 < NewbieImmunity()) {
                ADDMSG(&f->msgs, msg_message("newbieimmunity", "turns",
                    NewbieImmunity() - f->age - 1));
            }
        }
        if (*fptr == f) {
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

    init_order(ord);
    f = getfaction();

    if (f == NULL || is_monsters(f)) {
        cmistake(u, ord, 66, MSG_EVENT);
        return 0;
    }
    if (f == u->faction)
        return 0;

    s = gettoken(token, sizeof(token));

    if (!s[0])
        keyword = P_ANY;
    else
        keyword = findparam(s, u->faction->locale);

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

        if (!exist) {
            int key;
            for (key = 0; race_prefixes[key]; ++key) {
                variant var;
                const char *pname =
                    LOC(lang, mkname("prefix", race_prefixes[key]));
                if (findtoken(in->names, pname, &var) == E_TOK_NOMATCH || var.i != key) {
                    var.i = key;
                    addtoken(&in->names, pname, var);
                    addtoken(&in->names, LOC(lang, mkname("prefix",
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

    init_order(ord);
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
            group *g = (group *)a->data.v;
            if (a)
                ap = &g->attribs;
        }
        set_prefix(ap, race_prefixes[var.i]);
    }
    return 0;
}

static cmp_building_cb get_cmp_region_owner(void)
{
    if (rule_region_owners()) {
        return &cmp_current_owner;
    }
    else {
        return &cmp_wage;
    }
}

int display_cmd(unit * u, struct order *ord)
{
    char token[128];
    char **s = NULL;
    const char *str;
    region *r = u->region;

    init_order(ord);

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
        if (!u->building) {
            cmistake(u, ord, 145, MSG_EVENT);
            break;
        }
        if (building_owner(u->building) != u) {
            cmistake(u, ord, 148, MSG_EVENT);
            break;
        }
        if (u->building != largestbuilding(r, get_cmp_region_owner(), false)) {
            cmistake(u, ord, 147, MSG_EVENT);
            break;
        }
        s = &r->display;
        break;

    default:
        cmistake(u, ord, 110, MSG_EVENT);
        break;
    }

    if (s != NULL) {
        const char *s2 = getstrtoken();

        free(*s);
        if (s2) {
            *s = _strdup(s2);
            if (strlen(s2) >= DISPLAYSIZE) {
                (*s)[DISPLAYSIZE] = 0;
            }
        }
        else {
            *s = 0;
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
    if (!s2[0]) {
        cmistake(u, ord, 84, MSG_EVENT);
        return 0;
    }

    /* TODO: Validate to make sure people don't have illegal characters in
     * names, phishing-style? () come to mind. */

    free(*s);
    *s = _strdup(s2);
    if (strlen(s2) >= NAMESIZE) {
        (*s)[NAMESIZE] = 0;
    }
    return 0;
}

int
rename_building(unit * u, order * ord, building * b, const char *name)
{
    unit *owner = b ? building_owner(b) : 0;
    bool foreign = !(owner && owner->faction == u->faction);

    if (!b) {
        cmistake(u, ord, u->building ? 6 : 145, MSG_EVENT);
        return -1;
    }

    if (!fval(b->type, BTF_NAMECHANGE) && renamed_building(b)) {
        cmistake(u, ord, 278, MSG_EVENT);
        return -1;
    }

    if (foreign) {
        if (renamed_building(b)) {
            cmistake(u, ord, 246, MSG_EVENT);
            return -1;
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
        }
    }
    else {
        if (owner != u) {
            cmistake(u, ord, 148, MSG_PRODUCE);
            return -1;
        }
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

    init_order(ord);
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

        return rename_building(u, ord, b, getstrtoken());

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
                for (; lang; lang = nextlocale(lang)) {
                    const char *fdname = LOC(lang, "factiondefault");
                    size_t fdlen = strlen(fdname);
                    if (strlen(f->name) >= fdlen && strncmp(f->name, fdname, fdlen) == 0) {
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
                for (; lang; lang = nextlocale(lang)) {
                    const char *sdname = LOC(lang, sh->type->_name);
                    size_t sdlen = strlen(sdname);
                    if (strlen(sh->name) >= sdlen
                        && strncmp(sh->name, sdname, sdlen) == 0) {
                        break;
                    }

                    sdname = LOC(lang, parameters[P_SHIP]);
                    sdlen = strlen(sdname);
                    if (strlen(sh->name) >= sdlen
                        && strncmp(sh->name, sdname, sdlen) == 0) {
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
            if (!u->ship) {
                cmistake(u, ord, 144, MSG_PRODUCE);
                break;
            }
            if (ship_owner(u->ship) != u) {
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
                const char *udefault = LOC(u2->faction->locale, "unitdefault");
                size_t udlen = strlen(udefault);
                size_t unlen = strlen(u2->name);
                if (unlen >= udlen && strncmp(u2->name, udefault, udlen) != 0) {
                    cmistake(u2, ord, 244, MSG_EVENT);
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
            s = &u2->name;
        }
        else {
            s = &u->name;
        }
        break;

    case P_REGION:
        if (!b) {
            cmistake(u, ord, 145, MSG_EVENT);
            break;
        }
        if (building_owner(b) != u) {
            cmistake(u, ord, 148, MSG_EVENT);
            break;
        }

        if (b != largestbuilding(r, get_cmp_region_owner(), false)) {
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
        return rename_cmd(u, ord, s, getstrtoken());
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

    init_order(ord);
    s = gettoken(token, sizeof(token));

    /* Falls kein Parameter, ist das eine Einheitsnummer;
     * das Füllwort "AN" muß wegfallen, da gültige Nummer! */

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
            if (!s[0]) {
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
            if (!s[0]) {
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

            if (!s[0]) {
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

            if (!s[0]) {
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
    init_order(ord);
    const char * s = getstrtoken();

    free(u->faction->banner);
    u->faction->banner = s ? _strdup(s) : 0;
    add_message(&u->faction->msgs, msg_message("changebanner", "value",
        u->faction->banner));

    return 0;
}

int email_cmd(unit * u, struct order *ord)
{
    const char *s;

    init_order(ord);
    s = getstrtoken();

    if (!s || !s[0]) {
        cmistake(u, ord, 85, MSG_EVENT);
    }
    else {
        faction *f = u->faction;
        if (set_email(&f->email, s) != 0) {
            log_error("Invalid email address for faction %s: %s\n", itoa36(f->no), s);
            ADDMSG(&f->msgs, msg_message("changemail_invalid", "value", s));
        }
        else {
            ADDMSG(&f->msgs, msg_message("changemail", "value", f->email));
        }
    }
    return 0;
}

int password_cmd(unit * u, struct order *ord)
{
    char pwbuf[32];
    int i;
    const char *s;
    bool pwok = true;

    init_order(ord);
    s = gettoken(pwbuf, sizeof(pwbuf));

    if (!s || !*s) {
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
    free(u->faction->passw);
    if (!pwok) {
        cmistake(u, ord, 283, MSG_EVENT);
        u->faction->passw = _strdup(itoa36(rng_int()));
    }
    else {
        u->faction->passw = _strdup(pwbuf);
    }
    ADDMSG(&u->faction->msgs, msg_message("changepasswd",
        "value", u->faction->passw));
    return 0;
}

int send_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *s;
    int option;

    init_order(ord);
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

static bool display_item(faction * f, unit * u, const item_type * itype)
{
    const char *name;
    const char *key;
    const char *info;

    if (u != NULL) {
        int i = i_get(u->items, itype);
        if (i == 0) {
            if (u->region->land != NULL) {
                i = i_get(u->region->land->items, itype);
            }
            if (i == 0) {
                i = i_get(u->faction->items, itype);
                if (i == 0)
                    return false;
            }
        }
    }

    name = resourcename(itype->rtype, 0);
    key = mkname("iteminfo", name);
    info = locale_getstring(f->locale, key);

    if (info == NULL) {
        info = LOC(f->locale, mkname("iteminfo", "no_info"));
    }
    ADDMSG(&f->msgs, msg_message("displayitem", "weight item description",
        itype->weight, itype->rtype, info));

    return true;
}

static bool display_potion(faction * f, unit * u, const potion_type * ptype)
{
    attrib *a;

    if (ptype == NULL)
        return false;
    else {
        int i = i_get(u->items, ptype->itype);
        if (i == 0 && 2 * ptype->level > effskill(u, SK_ALCHEMY)) {
            return false;
        }
    }

    a = a_find(f->attribs, &at_showitem);
    while (a && a->data.v != ptype)
        a = a->next;
    if (!a) {
        a = a_add(&f->attribs, a_new(&at_showitem));
        a->data.v = (void *)ptype->itype;
    }

    return true;
}

static bool display_race(faction * f, unit * u, const race * rc)
{
    const char *name, *key;
    const char *info;
    int a, at_count;
    char buf[2048], *bufp = buf;
    size_t size = sizeof(buf) - 1;
    int bytes;

    if (u && u_race(u) != rc)
        return false;
    name = rc_name_s(rc, NAME_SINGULAR);

    bytes = slprintf(bufp, size, "%s: ", LOC(f->locale, name));
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    key = mkname("raceinfo", rc->_name);
    info = locale_getstring(f->locale, key);
    if (info == NULL) {
        info = LOC(f->locale, mkname("raceinfo", "no_info"));
    }

    bytes = (int)strlcpy(bufp, info, size);
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    /* hp_p : Trefferpunkte */
    bytes =
        slprintf(bufp, size, " %d %s", rc->hitpoints, LOC(f->locale,
        "stat_hitpoints"));
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    /* b_attacke : Angriff */
    bytes =
        slprintf(bufp, size, ", %s: %d", LOC(f->locale, "stat_attack"),
        (rc->at_default + rc->at_bonus));
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    /* b_defense : Verteidigung */
    bytes =
        slprintf(bufp, size, ", %s: %d", LOC(f->locale, "stat_defense"),
        (rc->df_default + rc->df_bonus));
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    /* b_armor : Rüstung */
    if (rc->armor > 0) {
        bytes =
            slprintf(bufp, size, ", %s: %d", LOC(f->locale, "stat_armor"), rc->armor);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }

    if (size > 1) {
        *bufp++ = '.';
        --size;
    }
    else
        WARN_STATIC_BUFFER();

    /* b_damage : Schaden */
    at_count = 0;
    for (a = 0; a < 6; a++) {
        if (rc->attack[a].type != AT_NONE) {
            at_count++;
        }
    }
    if (rc->battle_flags & BF_EQUIPMENT) {
        bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "stat_equipment"));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    if (rc->battle_flags & BF_RES_PIERCE) {
        bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "stat_pierce"));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    if (rc->battle_flags & BF_RES_CUT) {
        bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "stat_cut"));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }
    if (rc->battle_flags & BF_RES_BASH) {
        bytes = _snprintf(bufp, size, " %s", LOC(f->locale, "stat_bash"));
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
    }

    bytes =
        _snprintf(bufp, size, " %d %s", at_count, LOC(f->locale,
        (at_count == 1) ? "stat_attack" : "stat_attacks"));
    if (wrptr(&bufp, &size, bytes) != 0)
        WARN_STATIC_BUFFER();

    for (a = 0; a < 6; a++) {
        if (rc->attack[a].type != AT_NONE) {
            if (a != 0)
                bytes = (int)strlcpy(bufp, ", ", size);
            else
                bytes = (int)strlcpy(bufp, ": ", size);
            if (wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();

            switch (rc->attack[a].type) {
            case AT_STANDARD:
                bytes =
                    _snprintf(bufp, size, "%s (%s)",
                    LOC(f->locale, "attack_standard"), rc->def_damage);
                break;
            case AT_NATURAL:
                bytes =
                    _snprintf(bufp, size, "%s (%s)",
                    LOC(f->locale, "attack_natural"), rc->attack[a].data.dice);
                break;
            case AT_SPELL:
            case AT_COMBATSPELL:
            case AT_DRAIN_ST:
            case AT_DRAIN_EXP:
            case AT_DAZZLE:
                bytes = _snprintf(bufp, size, "%s", LOC(f->locale, "attack_magical"));
                break;
            case AT_STRUCTURAL:
                bytes =
                    _snprintf(bufp, size, "%s (%s)",
                    LOC(f->locale, "attack_structural"), rc->attack[a].data.dice);
                break;
            default:
                bytes = 0;
            }

            if (bytes && wrptr(&bufp, &size, bytes) != 0)
                WARN_STATIC_BUFFER();
        }
    }

    if (size > 1) {
        *bufp++ = '.';
        --size;
    }
    else
        WARN_STATIC_BUFFER();

    *bufp = 0;
    addmessage(0, f, buf, MSG_EVENT, ML_IMPORTANT);

    return true;
}

static void reshow(unit * u, struct order *ord, const char *s, param_t p)
{
    int skill, c;
    const potion_type *ptype;
    const item_type *itype;
    const spell *sp = 0;
    const race *rc;

    switch (p) {
    case P_ZAUBER:
        a_removeall(&u->faction->attribs, &at_seenspell);
        break;
    case P_POTIONS:
        skill = effskill(u, SK_ALCHEMY);
        c = 0;
        for (ptype = potiontypes; ptype != NULL; ptype = ptype->next) {
            if (ptype->level * 2 <= skill) {
                c += display_potion(u->faction, u, ptype);
            }
        }
        if (c == 0)
            cmistake(u, ord, 285, MSG_EVENT);
        break;
    case NOPARAM:
        if (s) {
            /* check if it's an item */
            itype = finditemtype(s, u->faction->locale);
            if (itype != NULL) {
                ptype = resource2potion(item2resource(itype));
                if (ptype != NULL) {
                    if (display_potion(u->faction, u, ptype))
                        break;
                }
                else {
                    if (display_item(u->faction, u, itype))
                        break;
                }
            }
            /* try for a spell */
            sp = unit_getspell(u, s, u->faction->locale);
            if (sp) {
                attrib *a = a_find(u->faction->attribs, &at_seenspell);
                while (a != NULL && a->type == &at_seenspell && a->data.v != sp) {
                    a = a->next;
                }
                if (a != NULL) {
                    a_remove(&u->faction->attribs, a);
                }
                break;
            }
            /* last, check if it's a race. */
            rc = findrace(s, u->faction->locale);
            if (rc != NULL && display_race(u->faction, u, rc)) {
                break;
            }
        }
        cmistake(u, ord, 21, MSG_EVENT);
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
    people = count_all(u->faction) * u->number;
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

    kwd = init_order(ord);
    assert(kwd == K_GROUP);
    join_group(u, getstrtoken());
    return 0;
}

int origin_cmd(unit * u, struct order *ord)
{
    short px, py;

    init_order(ord);

    px = (short)getint();
    py = (short)getint();

    set_ursprung(u->faction, getplaneid(u->region), px, py);
    return 0;
}

int guard_off_cmd(unit * u, struct order *ord)
{
    assert(getkeyword(ord) == K_GUARD);
    init_order(ord);

    if (getparam(u->faction->locale) == P_NOT) {
        setguard(u, GUARD_NONE);
    }
    return 0;
}

int reshow_cmd(unit * u, struct order *ord)
{
    char lbuf[64];
    const char *s;
    param_t p = NOPARAM;

    init_order(ord);
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

    init_order(ord);
    s = gettoken(token, sizeof(token));
    switch (findparam(s, u->faction->locale)) {
    case P_NOT:
        setstatus(u, ST_AVOID);
        break;
    case P_BEHIND:
        setstatus(u, ST_BEHIND);
        break;
    case P_FLEE:
        setstatus(u, ST_FLEE);
        break;
    case P_CHICKEN:
        setstatus(u, ST_CHICKEN);
        break;
    case P_AGGRO:
        setstatus(u, ST_AGGRO);
        break;
    case P_VORNE:
        setstatus(u, ST_FIGHT);
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
            setstatus(u, ST_FIGHT);
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

    init_order(ord);
    s = gettoken(token, sizeof(token));

    /* KAMPFZAUBER [NICHT] löscht alle gesetzten Kampfzauber */
    if (!s || *s == 0 || findparam(s, u->faction->locale) == P_NOT) {
        unset_combatspell(u, 0);
        return 0;
    }

    /* Optional: STUFE n */
    if (findparam(s, u->faction->locale) == P_LEVEL) {
        /* Merken, setzen kommt erst später */
        level = getint();
        level = _max(0, level);
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

/* ------------------------------------------------------------- */
/* Beachten: einige Monster sollen auch unbewaffent die Region bewachen
 * können */

enum { E_GUARD_OK, E_GUARD_UNARMED, E_GUARD_NEWBIE, E_GUARD_FLEEING };

static int can_start_guarding(const unit * u)
{
    if (u->status >= ST_FLEE)
        return E_GUARD_FLEEING;
    if (fval(u_race(u), RCF_UNARMEDGUARD))
        return E_GUARD_OK;
    if (!armedmen(u, true))
        return E_GUARD_UNARMED;
    if (IsImmune(u->faction))
        return E_GUARD_NEWBIE;
    return E_GUARD_OK;
}

void update_guards(void)
{
    const region *r;

    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (fval(u, UFL_GUARD)) {
                if (can_start_guarding(u) != E_GUARD_OK) {
                    setguard(u, GUARD_NONE);
                }
                else {
                    attrib *a = a_find(u->attribs, &at_guard);
                    if (a && a->data.i == (int)guard_flags(u)) {
                        /* this is really rather not necessary */
                        a_remove(&u->attribs, a);
                    }
                }
            }
        }
    }
}

int guard_on_cmd(unit * u, struct order *ord)
{
    assert(getkeyword(ord) == K_GUARD);

    init_order(ord);

    /* GUARD NOT is handled in goard_off_cmd earlier in the turn */
    if (getparam(u->faction->locale) == P_NOT)
        return 0;

    if (fval(u->region->terrain, SEA_REGION)) {
        cmistake(u, ord, 2, MSG_EVENT);
    }
    else {
        if (fval(u, UFL_MOVED)) {
            cmistake(u, ord, 187, MSG_EVENT);
        }
        else if (fval(u_race(u), RCF_ILLUSIONARY)
            || u_race(u) == get_race(RC_SPELL)) {
            cmistake(u, ord, 95, MSG_EVENT);
        }
        else {
            /* Monster der Monsterpartei dürfen immer bewachen */
            if (is_monsters(u->faction)) {
                guard(u, GUARD_ALL);
            }
            else {
                int err = can_start_guarding(u);
                if (err == E_GUARD_OK) {
                    guard(u, GUARD_ALL);
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
            if (fval(r->terrain, SEA_REGION) && (!enoughsailors(sh, r)
                || get_captain(sh) == NULL)) {
                /* Schiff nicht seetüchtig */
                float dmg = get_param_flt(global.parameters,
                    "rules.ship.damage.nocrewocean",
                    0.30F);
                damage_ship(sh, dmg);
            }
            if (ship_owner(sh) == NULL) {
                float dmg = get_param_flt(global.parameters, "rules.ship.damage.nocrew",
                    0.05F);
                damage_ship(sh, dmg);
            }
        }
        if (sh->damage >= sh->size * DAMAGE_SCALE) {
            remove_ship(shp, sh);
        }
        if (*shp == sh)
            shp = &sh->next;
    }
}

/* The following functions do not really belong here: */
#include <kernel/config.h>
#include <kernel/build.h>

static attrib_type at_number = {
    "faction_renum",
    NULL, NULL, NULL, NULL, NULL,
    ATF_UNIQUE
};

void renumber_factions(void)
/* gibt parteien neue nummern */
{
    struct renum {
        struct renum *next;
        int want;
        faction *faction;
        attrib *attrib;
    } *renum = NULL, *rp;
    faction *f;
    for (f = factions; f; f = f->next) {
        attrib *a = a_find(f->attribs, &at_number);
        int want;
        struct renum **rn;
        faction *old;

        if (!a)
            continue;
        want = a->data.i;
        if (fval(f, FFL_NEWID)) {
            ADDMSG(&f->msgs, msg_message("renumber_twice", "id", want));
            continue;
        }
        old = findfaction(want);
        if (old) {
            a_remove(&f->attribs, a);
            ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
            continue;
        }
        if (!faction_id_is_unused(want)) {
            a_remove(&f->attribs, a);
            ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
            continue;
        }
        for (rn = &renum; *rn; rn = &(*rn)->next) {
            if ((*rn)->want >= want)
                break;
        }
        if (*rn && (*rn)->want == want) {
            ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
        }
        else {
            struct renum *r = calloc(sizeof(struct renum), 1);
            r->next = *rn;
            r->attrib = a;
            r->faction = f;
            r->want = want;
            *rn = r;
        }
    }
    for (rp = renum; rp; rp = rp->next) {
        f = rp->faction;
        a_remove(&f->attribs, rp->attrib);
        renumber_faction(f, rp->want);
    }
    while (renum) {
        rp = renum->next;
        free(renum);
        renum = rp;
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

                        init_order(ord);
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
                            cmistake(u, ord, 10, MSG_EVENT);
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

int renumber_cmd(unit * u, order * ord)
{
    char token[128];
    const char *s;
    int i;
    faction *f = u->faction;

    init_order(ord);
    s = gettoken(token, sizeof(token));
    switch (findparam_ex(s, u->faction->locale)) {

    case P_FACTION:
        s = gettoken(token, sizeof(token));
        if (s && *s) {
            int id = atoi36((const char *)s);
            attrib *a = a_find(f->attribs, &at_number);
            if (!a)
                a = a_add(&f->attribs, a_new(&at_number));
            a->data.i = id;
        }
        break;

    case P_UNIT:
        s = gettoken(token, sizeof(token));
        if (s == NULL || *s == 0) {
            i = newunitid();
        }
        else {
            i = atoi36((const char *)s);
            if (i <= 0 || i > MAX_UNIT_NR) {
                cmistake(u, ord, 114, MSG_EVENT);
                break;
            }

            if (forbiddenid(i)) {
                cmistake(u, ord, 116, MSG_EVENT);
                break;
            }

            if (findunitg(i, u->region)) {
                cmistake(u, ord, 115, MSG_EVENT);
                break;
            }
        }
        uunhash(u);
        if (!ualias(u)) {
            attrib *a = a_add(&u->attribs, a_new(&at_alias));
            a->data.i = -u->no;
        }
        u->no = i;
        uhash(u);
        break;

    case P_SHIP:
        if (!u->ship) {
            cmistake(u, ord, 144, MSG_EVENT);
            break;
        }
        if (ship_owner(u->ship) != u) {
            cmistake(u, ord, 146, MSG_EVENT);
            break;
        }
        if (u->ship->coast != NODIRECTION) {
            cmistake(u, ord, 116, MSG_EVENT);
            break;
        }
        s = gettoken(token, sizeof(token));
        if (s == NULL || *s == 0) {
            i = newcontainerid();
        }
        else {
            i = atoi36((const char *)s);
            if (i <= 0 || i > MAX_CONTAINER_NR) {
                cmistake(u, ord, 114, MSG_EVENT);
                break;
            }
            if (findship(i) || findbuilding(i)) {
                cmistake(u, ord, 115, MSG_EVENT);
                break;
            }
        }
        sunhash(u->ship);
        u->ship->no = i;
        shash(u->ship);
        break;
    case P_BUILDING:
    case P_GEBAEUDE:
        if (!u->building) {
            cmistake(u, ord, 145, MSG_EVENT);
            break;
        }
        if (building_owner(u->building) != u) {
            cmistake(u, ord, 148, MSG_EVENT);
            break;
        }
        s = gettoken(token, sizeof(token));
        if (*s == 0) {
            i = newcontainerid();
        }
        else {
            i = atoi36((const char *)s);
            if (i <= 0 || i > MAX_CONTAINER_NR) {
                cmistake(u, ord, 114, MSG_EVENT);
                break;
            }
            if (findship(i) || findbuilding(i)) {
                cmistake(u, ord, 115, MSG_EVENT);
                break;
            }
        }
        bunhash(u->building);
        u->building->no = i;
        bhash(u->building);
        break;

    default:
        cmistake(u, ord, 239, MSG_EVENT);
    }
    return 0;
}

static building *age_building(building * b)
{
    const struct building_type *bt_blessed;
    const struct curse_type *ct_astralblock;
    const struct resource_type *rtype = get_resourcetype(R_UNICORN);

    bt_blessed = bt_find("blessedstonecircle");
    ct_astralblock = ct_find("astralblock");

    /* blesses stone circles create an astral protection in the astral region
     * above the shield, which prevents chaos suction and other spells.
     * The shield is created when a magician enters the blessed stone circle,
     * and lasts for as long as his skill level / 2 is, at no mana cost.
     *
     * TODO: this would be nicer in a btype->age function, but we don't have it.
     */
    if (rtype && ct_astralblock && bt_blessed && b->type == bt_blessed) {
        region *r = b->region;
        region *rt = r_standard_to_astral(r);
        unit *u, *mage = NULL;

        /* step 1: give unicorns to people in the building,
         * find out if there's a magician in there. */
        for (u = r->units; u; u = u->next) {
            if (b == u->building && inside_building(u)) {
                if (!(u_race(u)->ec_flags & GIVEITEM) == 0) {
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
                if (mage == NULL && is_mage(u)) {
                    mage = u;
                }
            }
        }

        /* if there's a magician, and a connection to astral space, create the
         * curse. */
        if (rt && !fval(rt->terrain, FORBIDDEN_REGION) && mage != NULL) {
            curse *c = get_curse(rt->attribs, ct_astralblock);
            if (c == NULL) {
                if (mage != NULL) {
                    int sk = effskill(mage, SK_MAGIC);
                    float effect = 100;
                    /* the mage reactivates the circle */
                    c = create_curse(mage, &rt->attribs, ct_astralblock,
                        (float)_max(1, sk), _max(1, sk / 2), effect, 0);
                    ADDMSG(&r->msgs,
                        msg_message("astralshield_activate", "region unit", r, mage));
                }
            }
            else if (mage != NULL) {
                int sk = effskill(mage, SK_MAGIC);
                c->duration = _max(c->duration, sk / 2);
                c->vigour = _max(c->vigour, sk);
            }
        }
    }

    a_age(&b->attribs);
    handle_event(b->attribs, "timer", b);

    if (b->type->age) {
        b->type->age(b);
    }

    return b;
}

static void age_region(region * r)
{
    a_age(&r->attribs);
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
                change_effect(u, oldpotiontype[P_STRONG], -1 * _min(u->number, i));
            }
            /* Berserkerblut */
            i = get_effect(u, oldpotiontype[P_BERSERK]);
            if (i > 0) {
                change_effect(u, oldpotiontype[P_BERSERK], -1 * _min(u->number, i));
            }

            if (is_cursed(u->attribs, C_OLDRACE, 0)) {
                curse *c = get_curse(u->attribs, ct_find("oldrace"));
                if (c->duration == 1 && !(c_flags(c) & CURSE_NOAGE)) {
                    u_setrace(u, get_race(curse_geteffect_int(c)));
                    u->irace = NULL;
                }
            }
        }
    }

    /* Borders */
    age_borders();

    /* Factions */
    for (f = factions; f; f = f->next) {
        a_age(&f->attribs);
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
            a_age(&u->attribs);
            if (u == *up)
                handle_event(u->attribs, "timer", u);
            if (u == *up)
                up = &(*up)->next;
        }

        /* Schiffe */
        for (sp = &r->ships; *sp;) {
            ship *s = *sp;
            a_age(&s->attribs);
            if (s == *sp)
                handle_event(s->attribs, "timer", s);
            if (s == *sp)
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
    if (alimit == 0) {
        return flimit;
    }
    if (flimit == 0) {
        return alimit;
    }
    return _min(alimit, flimit);
}

int checkunitnumber(const faction * f, int add)
{
    int alimit, flimit;
    int flags = COUNT_DEFAULT | COUNT_MIGRANTS | COUNT_UNITS;
    int fno = count_faction(f, flags) + add;
    flimit = rule_faction_limit();
    if (flimit && fno > flimit) {
        return 2;
    }

    alimit = rule_alliance_limit();
    if (alimit) {
        /* if unitsperalliance is true, maxunits returns the
         number of units allowed in an alliance */
        faction *f2;
        int unitsinalliance = fno;
        if (unitsinalliance > alimit) {
            return 1;
        }

        for (f2 = factions; f2; f2 = f2->next) {
            if (f != f2 && f->alliance == f2->alliance) {
                unitsinalliance += count_faction(f2, flags);
                if (unitsinalliance > alimit) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void new_units(void)
{
    region *r;
    unit *u, *u2;

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
                order *makeord = *ordp;
                if (getkeyword(makeord) == K_MAKETEMP) {
                    char token[128], *name = NULL;
                    const char *s;
                    int alias;
                    ship *sh;
                    order **newordersp;
                    int err = checkunitnumber(u->faction, 1);

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
                        ordp = &makeord->next;

                        while (*ordp) {
                            order *ord = *ordp;
                            if (getkeyword(ord) == K_END)
                                break;
                            *ordp = ord->next;
                            ord->next = NULL;
                            free_order(ord);
                        }
                        continue;
                    }
                    init_order(makeord);
                    alias = getid();

                    s = gettoken(token, sizeof(token));
                    if (s && s[0]) {
                        name = _strdup(s);
                    }
                    u2 = create_unit(r, u->faction, 0, u->faction->race, alias, name, u);
                    if (name != NULL)
                        free(name);
                    fset(u2, UFL_ISNEW);

                    a_add(&u2->attribs, a_new(&at_alias))->data.i = alias;
                    sh = leftship(u);
                    if (sh) {
                        set_leftship(u2, sh);
                    }
                    setstatus(u2, u->status);

                    ordp = &makeord->next;
                    newordersp = &u2->orders;
                    while (*ordp) {
                        order *ord = *ordp;
                        if (getkeyword(ord) == K_END)
                            break;
                        *ordp = ord->next;
                        ord->next = NULL;
                        *newordersp = ord;
                        newordersp = &ord->next;
                    }
                }
                if (*ordp == makeord)
                    ordp = &makeord->next;
            }
        }
    }
}

/** Checks for two long orders and issues a warning if necessary.
 */
void check_long_orders(unit * u)
{
    order *ord;
    keyword_t otherorder = MAXKEYWORDS;

    for (ord = u->orders; ord; ord = ord->next) {
        if (getkeyword(ord) == NOKEYWORD) {
            cmistake(u, ord, 22, MSG_EVENT);
        }
        else if (is_long(ord)) {
            keyword_t longorder = getkeyword(ord);
            if (otherorder != MAXKEYWORDS) {
                switch (longorder) {
                case K_CAST:
                    if (otherorder != longorder) {
                        cmistake(u, ord, 52, MSG_EVENT);
                    }
                    break;
                case K_BUY:
                    if (otherorder == K_SELL) {
                        otherorder = K_BUY;
                    }
                    else {
                        cmistake(u, ord, 52, MSG_EVENT);
                    }
                    break;
                case K_SELL:
                    if (otherorder != K_SELL && otherorder != K_BUY) {
                        cmistake(u, ord, 52, MSG_EVENT);
                    }
                    break;
                default:
                    cmistake(u, ord, 52, MSG_EVENT);
                }
            }
            else {
                otherorder = longorder;
            }
        }
    }
}

void update_long_order(unit * u)
{
    order *ord;
    bool trade = false;
    bool hunger = LongHunger(u);

    freset(u, UFL_MOVED);
    freset(u, UFL_LONGACTION);
    if (hunger) {
        /* Hungernde Einheiten führen NUR den default-Befehl aus */
        set_order(&u->thisorder, default_order(u->faction->locale));
    }
    else {
        check_long_orders(u);
    }
    /* check all orders for a potential new long order this round: */
    for (ord = u->orders; ord; ord = ord->next) {
        if (getkeyword(ord) == NOKEYWORD)
            continue;

        if (u->old_orders && is_repeated(ord)) {
            /* this new order will replace the old defaults */
            free_orders(&u->old_orders);
            if (hunger)
                break;
        }
        if (hunger)
            continue;

        if (is_exclusive(ord)) {
            /* Über dieser Zeile nur Befehle, die auch eine idle Faction machen darf */
            if (idle(u->faction)) {
                set_order(&u->thisorder, default_order(u->faction->locale));
            }
            else {
                set_order(&u->thisorder, copy_order(ord));
            }
            break;
        }
        else {
            keyword_t keyword = getkeyword(ord);
            switch (keyword) {
                /* Wenn gehandelt wird, darf kein langer Befehl ausgeführt
                 * werden. Da Handel erst nach anderen langen Befehlen kommt,
                 * muß das vorher abgefangen werden. Wir merken uns also
                 * hier, ob die Einheit handelt. */
            case K_BUY:
            case K_SELL:
                /* Wenn die Einheit handelt, muß der Default-Befehl gelöscht
                 * werden.
                 * Wird je diese Ausschliesslichkeit aufgehoben, muss man aufpassen
                 * mit der Reihenfolge von Kaufen, Verkaufen etc., damit es Spielern
                 * nicht moeglich ist, Schulden zu machen. */
                trade = true;
                break;

            case K_CAST:
                /* dient dazu, das neben Zaubern kein weiterer Befehl
                 * ausgeführt werden kann, Zaubern ist ein kurzer Befehl */
                set_order(&u->thisorder, copy_order(ord));
                break;

            default:
                break;
            }
        }
    }

    if (hunger) {
        return;
    }
    /* Wenn die Einheit handelt, muß der Default-Befehl gelöscht
     * werden. */

    if (trade) {
        /* fset(u, UFL_LONGACTION|UFL_NOTMOVING); */
        set_order(&u->thisorder, NULL);
    }
}

static int
use_item(unit * u, const item_type * itype, int amount, struct order *ord)
{
    int i;
    int target = read_unitid(u->faction, u->region);

    i = get_pooled(u, itype->rtype, GET_DEFAULT, amount);

    if (amount > i) {
        amount = i;
    }
    if (amount == 0) {
        return ENOITEM;
    }

    if (target == -1) {
        if (itype->use == NULL) {
            return EUNUSABLE;
        }
        return itype->use(u, itype, amount, ord);
    }
    else {
        if (itype->useonother == NULL) {
            return EUNUSABLE;
        }
        return itype->useonother(u, target, itype, amount, ord);
    }
}

static double heal_factor(const unit * u)
{
    static float elf_regen = -1;
    switch (old_race(u_race(u))) {
    case RC_TROLL:
    case RC_DAEMON:
        return 1.5;
    case RC_GOBLIN:
        return 2.0;
    case RC_ELF:
        if (elf_regen < 0)
            elf_regen = get_param_flt(u_race(u)->parameters, "regen.forest", 1.0F);
        if (elf_regen != 1.0 && r_isforest(u->region)) {
            return elf_regen;
        }
        return 1.0;
    default:
        return 1.0;
    }
}

void monthly_healing(void)
{
    region *r;
    static const curse_type *heal_ct = NULL;
    if (heal_ct == NULL)
        heal_ct = ct_find("healingzone");

    for (r = regions; r; r = r->next) {
        unit *u;
        double healingcurse = 0;

        if (heal_ct != NULL) {
            /* bonus zurücksetzen */
            curse *c = get_curse(r->attribs, heal_ct);
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

            p *= heal_factor(u);
            if (u->hp < umhp) {
                double maxheal = _max(u->number, umhp / 20.0);
                int addhp;
                struct building *b = inside_building(u);
                const struct building_type *btype = b ? b->type : NULL;
                if (btype == bt_find("inn")) {
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
                u->hp = _min(u->hp + addhp, umhp);

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
                    init_order(ord);
                    s = gettoken(lbuf, sizeof(lbuf));
                    if (s) {
                        new_order = parse_order(s, u->faction->locale);
                    }
                    *ordp = ord->next;
                    ord->next = NULL;
                    free_order(ord);
                    if (!neworders) {
                        /* lange Befehle aus orders und old_orders löschen zu gunsten des neuen */
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
            sc_mage *mage = get_mage(u);
            if (mage) {
                int level = eff_skill(u, SK_MAGIC, u->region);
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
        quicklist *ql;
        int qi;
        for (qi = 0, ql = src->spells; ql; ql_advance(&ql, &qi, 1)) {
            spellbook_entry * sbe = (spellbook_entry *)ql_get(ql, qi);
            if (sbe->level <= maxlevel) {
                if (!spellbook_get(dst, sbe->sp)) {
                    spellbook_add(dst, sbe->sp, sbe->level);
                }
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
                sc_mage *mage = get_mage(u);
                if (mage && mage->spellbook) {
                    int level = effskill(u, SK_MAGIC);
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

    init_order(ord);

    t = gettoken(token, sizeof(token));
    n = atoi((const char *)t);
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
        assert(err <= 0 || !"use_item should not return positive values.");
        if (err > 0) {
            log_error("use_item returned a value>0 for %s\n", resourcename(itype->rtype, 0));
        }
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
    }
    return err;
}

int pay_cmd(unit * u, struct order *ord)
{
    if (!u->building) {
        cmistake(u, ord, 6, MSG_EVENT);
    }
    else {
        param_t p;
        int id;
        init_order(ord);
        p = getparam(u->faction->locale);
        id = getid();
        building *b = NULL;
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
    if (u->number > 0 && (urace(u)->ec_flags & GETITEM)) {
        int use, count, para;
        const item_type *itype;
        const char *s;

        init_order(ord);
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
    int n;
    const item_type *itype;

    init_order(ord);

    t = gettoken(token, sizeof(token));
    n = atoi((const char *)t);
    if (n == 0) {
        n = 1;
    }
    else {
        t = gettoken(token, sizeof(token));
    }
    itype = finditemtype(t, u->faction->locale);

    if (itype != NULL) {
        item **iclaim = i_find(&u->faction->items, itype);
        if (iclaim != NULL && *iclaim != NULL) {
            n = _min(n, (*iclaim)->number);
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

typedef enum { PR_GLOBAL, PR_REGION_PRE, PR_UNIT, PR_ORDER, PR_REGION_POST } processor_t;

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

        if (verbosity >= 3)
            printf("- Step %u\n", prio);
        while (proc && proc->priority == prio) {
            if (proc->name && verbosity >= 1)
                log_printf(stdout, " - %s\n", proc->name);
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
                                        && !is_cursed(u->attribs, C_KAELTESCHUTZ, 0)) {
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

    if (verbosity >= 3)
        printf("\n - Leere Gruppen loeschen...\n");
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
    if (!(urace(u)->flags & RCF_NOWEAPONS)) {
        if (effskill(u, SK_WEAPONLESS) >= 1) {
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
                if (effskill(u, wtype->skill) >= wtype->minskill)
                    n += itm->number;
                /* if (effskill(u, wtype->skill) >= wtype->minskill) n += itm->number; */
                if (n > u->number)
                    break;
            }
            n = _min(n, u->number);
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
    const curse_type *magicwalls_ct;
    resource_type *rt_catapultammo = NULL;
    resource_type *rt_catapult = NULL;

    init_order(ord);
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

    magicwalls_ct = ct_find("magicwalls");
    rt_catapultammo = rt_find("catapultammo");
    rt_catapult = rt_find("catapult");

    d = i_get(u->items, rt_catapult->itype);
    d = _min(u->number, d);
    pooled = get_pooled(u, rt_catapultammo, GET_DEFAULT, d);
    d = _min(pooled, d);
    if (eff_skill(u, SK_CATAPULT, r) >= 1) {
        katapultiere = d;
        d *= eff_skill(u, SK_CATAPULT, r);
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

    if (!is_guard(u, GUARD_TRAVELTHRU)) {
        /* abbruch, wenn die einheit nicht vorher die region bewacht - als
         * warnung fuer alle anderen! */
        cmistake(u, ord, 81, MSG_EVENT);
        return 81;
    }
    /* einheit und burg markieren - spart zeit beim behandeln der einheiten
     * in der burg, falls die burg auch markiert ist und nicht alle
     * einheiten wieder abgesucht werden muessen! */

    usetsiege(u, b);
    b->besieged += _max(bewaffnete, katapultiere);

    /* definitiver schaden eingeschraenkt */

    d = _min(d, b->size - 1);

    /* meldung, schaden anrichten */
    if (d && !curse_active(get_curse(b->attribs, magicwalls_ct))) {
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

static bool help_enter(unit *uo, unit *u) {
    return uo->faction == u->faction || alliedunit(uo, u->faction, HELP_GUARD);
}

void force_leave(region *r) {
    unit *u;
    for (u = r->units; u; u = u->next) {
        unit *uo = NULL;
        if (u->building) {
            uo = building_owner(u->building);
        }
        if (u->ship && r->land) {
            uo = ship_owner(u->ship);
        }
        if (uo && !help_enter(uo, u)) {
            message *msg = NULL;
            if (u->building) {
                msg = msg_message("force_leave_building", "unit owner building", u, uo, u->building);
            }
            else {
                msg = msg_message("force_leave_ship", "unit owner ship", u, uo, u->ship);
            }
            if (msg) {
                ADDMSG(&u->faction->msgs, msg);
            }
            leave(u, false);
        }
    }
}

static void maintain_buildings_1(region * r)
{
    maintain_buildings(r, false);
}

/** warn about passwords that are not US ASCII.
 * even though passwords are technically UTF8 strings, the server receives
 * them as part of the Subject of an email when reports are requested.
 * This means that we need to limit them to ASCII characters until that
 * mechanism has been changed.
 */
static int warn_password(void)
{
    faction *f = factions;
    while (f) {
        bool pwok = true;
        const char *c = f->passw;
        while (*c && pwok) {
            if (!isalnum((unsigned char)*c))
                pwok = false;
            c++;
        }
        if (!pwok) {
            free(f->passw);
            f->passw = _strdup(itoa36(rng_int()));
            ADDMSG(&f->msgs, msg_message("illegal_password", "newpass", f->passw));
        }
        f = f->next;
    }
    return 0;
}

void init_processor(void)
{
    int p;

    p = 10;
    add_proc_global(p, new_units, "Neue Einheiten erschaffen");

    p += 10;
    add_proc_unit(p, update_long_order, "Langen Befehl aktualisieren");
    add_proc_order(p, K_BANNER, banner_cmd, 0, NULL);
    add_proc_order(p, K_EMAIL, email_cmd, 0, NULL);
    add_proc_order(p, K_PASSWORD, password_cmd, 0, NULL);
    add_proc_order(p, K_SEND, send_cmd, 0, NULL);
    add_proc_order(p, K_GROUP, group_cmd, 0, NULL);

    p += 10;
    add_proc_order(p, K_QUIT, &quit_cmd, 0, NULL);
    add_proc_order(p, K_URSPRUNG, &origin_cmd, 0, NULL);
    add_proc_order(p, K_ALLY, &ally_cmd, 0, NULL);
    add_proc_order(p, K_PREFIX, &prefix_cmd, 0, NULL);
    add_proc_order(p, K_SETSTEALTH, &setstealth_cmd, 0, NULL);
    add_proc_order(p, K_STATUS, &status_cmd, 0, NULL);
    add_proc_order(p, K_COMBATSPELL, &combatspell_cmd, 0, NULL);
    add_proc_order(p, K_DISPLAY, &display_cmd, 0, NULL);
    add_proc_order(p, K_NAME, &name_cmd, 0, NULL);
    add_proc_order(p, K_GUARD, &guard_off_cmd, 0, NULL);
    add_proc_order(p, K_RESHOW, &reshow_cmd, 0, NULL);

    if (get_param_int(global.parameters, "rules.alliances", 0) == 1) {
        p += 10;
        add_proc_global(p, alliance_cmd, NULL);
    }

    p += 10;
    add_proc_region(p, do_contact, "Kontaktieren");
    add_proc_order(p, K_MAIL, mail_cmd, 0, "Botschaften");

    p += 10;                      /* all claims must be done before we can USE */
    add_proc_region(p, &enter_1, "Betreten (1. Versuch)");     /* for GIVE CONTROL */
    add_proc_order(p, K_USE, use_cmd, 0, "Benutzen");

    p += 10;                      /* in case it has any effects on alliance victories */
    add_proc_order(p, K_GIVE, give_control_cmd, 0, "GIB KOMMANDO");

    p += 10;                      /* in case it has any effects on alliance victories */
    add_proc_order(p, K_LEAVE, leave_cmd, 0, "Verlassen");

    p += 10;
    add_proc_region(p, enter_1, "Betreten (2. Versuch)"); /* to allow a buildingowner to enter the castle pre combat */

    p += 10;
    add_proc_region(p, do_battle, "Attackieren");

    if (!keyword_disabled(K_BESIEGE)) {
        p += 10;
        add_proc_region(p, do_siege, "Belagern");
    }

    p += 10;                      /* can't allow reserve before siege (weapons) */
    add_proc_region(p, enter_1, "Betreten (3. Versuch)");  /* to claim a castle after a victory and to be able to DESTROY it in the same turn */
    if (get_param_int(global.parameters, "rules.reserve.twophase", 0)) {
        add_proc_order(p, K_RESERVE, &reserve_self, 0, "RESERVE (self)");
        p += 10;
    }
    add_proc_order(p, K_RESERVE, &reserve_cmd, 0, "RESERVE (all)");
    add_proc_order(p, K_CLAIM, &claim_cmd, 0, NULL);
    add_proc_unit(p, follow_unit, "Folge auf Einheiten setzen");

    p += 10;                      /* rest rng again before economics */
    if (get_param_int(global.parameters, "rules.owners.force_leave", 0)) {
        add_proc_region(p, force_leave, "kick non-allies out of buildings/ships");
    }
    add_proc_region(p, economics, "Zerstoeren, Geben, Rekrutieren, Vergessen");
    add_proc_order(p, K_PROMOTION, &promotion_cmd, 0, "Heldenbefoerderung");

    p += 10;
    if (!keyword_disabled(K_PAY)) {
        add_proc_order(p, K_PAY, &pay_cmd, 0, "Gebaeudeunterhalt (disable)");
    }
    add_proc_postregion(p, &maintain_buildings_1,
        "Gebaeudeunterhalt (1. Versuch)");

    p += 10;                      /* QUIT fuer sich alleine */
    add_proc_global(p, quit, "Sterben");

    if (!keyword_disabled(K_CAST)) {
        p += 10;
        add_proc_global(p, &magic, "Zaubern");
    }

    p += 10;
    add_proc_order(p, K_TEACH, &teach_cmd, PROC_THISORDER | PROC_LONGORDER,
        "Lehren");
    p += 10;
    add_proc_order(p, K_STUDY, &learn_cmd, PROC_THISORDER | PROC_LONGORDER,
        "Lernen");

    p += 10;
    add_proc_order(p, K_MAKE, &make_cmd, PROC_THISORDER | PROC_LONGORDER,
        "Produktion");
    add_proc_postregion(p, &produce, "Arbeiten, Handel, Rekruten");
    add_proc_postregion(p, &split_allocations, "Produktion II");

    p += 10;
    add_proc_region(p, &enter_2, "Betreten (4. Versuch)"); /* Once again after QUIT */

    p += 10;
    add_proc_region(p, &sinkships, "Schiffe sinken");

    p += 10;
    add_proc_global(p, &movement, "Bewegungen");

    if (get_param_int(global.parameters, "work.auto", 0)) {
        p += 10;
        add_proc_region(p, &auto_work, "Arbeiten (auto)");
    }

    p += 10;
    add_proc_order(p, K_GUARD, &guard_on_cmd, 0, "Bewache (an)");

    if (get_param_int(global.parameters, "rules.encounters", 0)) {
        p += 10;
        add_proc_global(p, &encounters, "Zufallsbegegnungen");
    }

    p += 10;
    add_proc_unit(p, &monster_kills_peasants,
        "Monster fressen und vertreiben Bauern");

    p += 10;
    add_proc_global(p, &randomevents, "Zufallsereignisse");

    p += 10;

    add_proc_global(p, &monthly_healing, "Regeneration (HP)");
    add_proc_global(p, &regenerate_aura, "Regeneration (Aura)");
    if (!keyword_disabled(K_DEFAULT)) {
        add_proc_global(p, &defaultorders, "Defaults setzen");
    }
    add_proc_global(p, &demographics, "Nahrung, Seuchen, Wachstum, Wanderung");

    if (!keyword_disabled(K_SORT)) {
        p += 10;
        add_proc_global(p, restack_units, "Einheiten sortieren");
    }
    if (!keyword_disabled(K_NUMBER)) {
        add_proc_order(p, K_NUMBER, &renumber_cmd, 0, "Neue Nummern (Einheiten)");
        p += 10;
        add_proc_global(p, &renumber_factions, "Neue Nummern");
    }
}

void processorders(void)
{
    static int init = 0;

    if (!init) {
        init_processor();
        init = 1;
    }
    update_spells();
    process();
    /*************************************************/

    if (get_param_int(global.parameters, "modules.markets", 0)) {
        do_markets();
    }

    log_info(" - Attribute altern");
    ageing();
    remove_empty_units();

    /* must happen AFTER age, because that would destroy them right away */
    if (get_param_int(global.parameters, "modules.wormholes", 0)) {
        wormholes_update();
    }

    /* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
     * Beschreibungen geändert haben */
    update_spells();
    warn_password();
}

int writepasswd(void)
{
    FILE *F;
    char zText[128];

    sprintf(zText, "%s/passwd", basepath());
    F = fopen(zText, "w");
    if (!F) {
        perror(zText);
    }
    else {
        faction *f;
        log_info("writing passwords...");

        for (f = factions; f; f = f->next) {
            fprintf(F, "%s:%s:%s:%u\n",
                factionid(f), f->email, f->passw, f->subscription);
        }
        fclose(F);
        return 0;
    }
    return 1;
}

void update_subscriptions(void)
{
    FILE *F;
    char zText[MAX_PATH];
    faction *f;
    strcat(strcpy(zText, basepath()), "/subscriptions");
    F = fopen(zText, "r");
    if (F == NULL) {
        log_warning(0, "could not open %s.\n", zText);
        return;
    }
    for (;;) {
        char zFaction[5];
        int subscription, fno;
        if (fscanf(F, "%d %s", &subscription, zFaction) <= 0)
            break;
        fno = atoi36(zFaction);
        f = findfaction(fno);
        if (f != NULL) {
            f->subscription = subscription;
        }
    }
    fclose(F);

    sprintf(zText, "subscriptions.%u", turn);
    F = fopen(zText, "w");
    for (f = factions; f != NULL; f = f->next) {
        fprintf(F, "%s:%u:%s:%s:%u:\n",
            itoa36(f->no), f->subscription, f->email, dbrace(f->race), f->lastorders);
    }
    fclose(F);
}

bool
cansee(const faction * f, const region * r, const unit * u, int modifier)
/* r kann != u->region sein, wenn es um durchreisen geht */
/* und es muss niemand aus f in der region sein, wenn sie vom Turm
* erblickt wird */
{
    int stealth, rings;
    unit *u2 = r->units;

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

    if (leftship(u))
        return true;

    while (u2 && u2->faction != f)
        u2 = u2->next;
    if (u2 == NULL)
        return false;

    /* simple visibility, just gotta have a unit in the region to see 'em */
    if (is_guard(u, GUARD_ALL) != 0 || usiege(u) || u->building || u->ship) {
        return true;
    }

    rings = invisible(u, NULL);
    stealth = eff_stealth(u, r) - modifier;

    while (u2) {
        if (rings < u->number || invisible(u, u2) < u->number) {
            if (skill_enabled(SK_PERCEPTION)) {
                int observation = eff_skill(u2, SK_PERCEPTION, r);

                if (observation >= stealth) {
                    return true;
                }
            }
            else {
                return true;
            }
        }

        /* find next unit in our faction */
        do {
            u2 = u2->next;
        } while (u2 && u2->faction != f);
    }
    return false;
}

bool cansee_unit(const unit * u, const unit * target, int modifier)
/* target->region kann != u->region sein, wenn es um durchreisen geht */
{
    if (fval(u_race(target), RCF_INVISIBLE) || target->number == 0)
        return false;
    else if (target->faction == u->faction)
        return true;
    else {
        int n, rings, o;

        if (is_guard(target, GUARD_ALL) != 0 || usiege(target) || target->building
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
            o = eff_skill(u, SK_PERCEPTION, target->region);
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
    int n;
    unit *u2;

    if (fval(u_race(u), RCF_INVISIBLE) || u->number == 0)
        return false;
    else if (u->faction == f)
        return true;
    else {
        int rings;

        if (is_guard(u, GUARD_ALL) != 0 || usiege(u) || u->building || u->ship) {
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

                o = eff_skill(u2, SK_PERCEPTION, r);

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
