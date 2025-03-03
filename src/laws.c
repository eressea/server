#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <kernel/config.h>
#include "laws.h"

#include "defaults.h"

#include <modules/gmcmd.h>

#include "alchemy.h"
#include "automate.h"
#include "battle.h"
#include "contact.h"
#include "economy.h"
#include "give.h"
#include "market.h"
#include "morale.h"
#include "monsters.h"
#include "move.h"
#include "randenc.h"
#include "recruit.h"
#include "renumber.h"
#include "sort.h"
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
#include <kernel/attrib.h>
#include "kernel/building.h"
#include "kernel/calendar.h"
#include "kernel/callbacks.h"
#include "kernel/connection.h"
#include "kernel/curse.h"
#include <kernel/event.h>
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
#include "kernel/skills.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"
#include "kernel/terrain.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"

/* util includes */
#include <util/base36.h>
#include <util/functions.h>
#include <util/goodies.h>
#include "util/keyword.h"
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/password.h>
#include <util/path.h>
#include <util/rand.h>
#include <util/rng.h>
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

#include <iniparser.h>
#include <selist.h>
#include <strings.h>

#include <stb_ds.h>

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

param_t findparam_ex(const char *s, const struct locale * lang)
{
    param_t result = get_param(s, lang);

    if (result == NOPARAM) {
        const building_type *btype = findbuildingtype(s, lang);
        if (btype != NULL)
            return P_GEBAEUDE;
    }
    return (result == P_BUILDING) ? P_GEBAEUDE : result;
}

int NewbieImmunity(void)
{
    int result = config_get_int("NewbieImmunity", 0);
    return result;
}

bool IsImmune(const faction * f, int age)
{
    return !fval(f, FFL_NPC) && age < NewbieImmunity();
}

int NMRTimeout(void)
{
    static int config, nmr_timeout, ini_timeout;
    if (config_changed(&config)) {
        nmr_timeout = config_get_int("nmr.timeout", 0);
        ini_timeout = config_get_int("game.maxnmr", 0);
    }
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

static void potion_effects(unit *u) {
    int effect = get_effect(u, oldpotiontype[P_FOOL]);
    if (effect > 0) {           /* Trank "Dumpfbackenbrot" */
        int weeks = u->number;
        if (weeks > effect) weeks = effect;
        ptrdiff_t s, n = arrlen(u->skills);
        skill *sb = NULL;
        for (s = 0; s != n; ++s) {
            skill* sv = u->skills + s;
            if (sb == NULL || skill_compare(sv, sb) > 0) {
                sb = sv;
            }
            ++sv;
        }
        /* bestes Talent raussuchen */
        if (sb != NULL) {
            reduce_skill_weeks(u, sb, weeks);
            ADDMSG(&u->faction->msgs, msg_message("dumbeffect",
                "unit weeks skill", u, weeks, (skill_t)sb->id));
        }                         /* sonst Glueck gehabt: wer nix weiss, kann nix vergessen... */
        change_effect(u, oldpotiontype[P_FOOL], -weeks);
    }
}

static void astral_crumble(unit *u) {
    item **itemp = &u->items;
    while (*itemp) {
        item *itm = *itemp;
        if ((itm->type->flags & ITF_NOTLOST) == 0) {
            if (itm->type->flags & (ITF_BIG | ITF_ANIMAL)) {
                ADDMSG(&u->faction->msgs, msg_message("itemcrumble",
                    "unit region item amount",
                    u, u->region, itm->type->rtype, itm->number));
                *itemp = itm->next;
                i_free(itm);
                continue;
            }
        }
        itemp = &itm->next;
    }
}

void age_unit(unit ** up)
{
    unit *u = *up;
    const race *rc = u_race(u);

    ++u->age;
    if (u->number > 0 && rc->age_unit) {
        rc->age_unit(u);
    }
    if (u->attribs) {
        potion_effects(u);
    }
    if (u->items && u->region && is_astral(u->region)) {
        astral_crumble(u);
    }
    a_age(&u->attribs, u);
    if (*up == u) {
        handle_event(u->attribs, "timer", u);
    }
}

static void live(region * r)
{
    get_food(r);
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

void peasant_migration(region * r)
{
    int i;
    int maxp = region_production(r);
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

        if (rc != NULL && rc->land) {
            int rp2 = rpeasants(rc);
            int maxp2 = region_production(rc);
            int max_emigration = MAX_EMIGRATION(rp2 - maxp2);

            if (max_emigration > 0) {
                if (max_emigration > max_immigrants) max_emigration = max_immigrants;
                if (max_emigration + r->land->newpeasants > USHRT_MAX) {
                    max_emigration = USHRT_MAX - r->land->newpeasants;
                }
                if (max_emigration + rc->land->newpeasants > USHRT_MAX) {
                    max_emigration = USHRT_MAX - rc->land->newpeasants;
                }
                r->land->newpeasants += (short)max_emigration;
                rc->land->newpeasants -= (short)max_emigration;
                max_immigrants -= max_emigration;
            }
        }
    }
}

/* Vermehrungsrate Bauern in 1/10000.
* TODO: Evt. Berechnungsfehler, reale Vermehrungsraten scheinen hoeher. */
#define PEASANTGROWTH 10
#define PEASANTLUCK 10
#define PEASANTFORCE 0.75       /* Chance einer Vermehrung trotz 90% Auslastung */

static double peasant_growth_factor(void)
{
    return config_get_flt("rules.peasants.growth.factor", 0.0001 * (double)PEASANTGROWTH);
}

static double peasant_luck_factor(void)
{
    return config_get_flt("rules.peasants.peasantluck.factor", PEASANTLUCK);
}

#define ROUND_BIRTHS(growth) (int)ceil(growth)

int peasant_luck_effect(int peasants, int luck, int maxp, double variance)
{
    int births = 0;
    double mean;
    if (luck == 0) return 0;
    mean = fmin(luck, peasants);
    mean *= peasant_luck_factor() * peasant_growth_factor();
    mean *= ((peasants / (double)maxp < .9) ? 1 : PEASANTFORCE);

    births = ROUND_BIRTHS(normalvariate(mean, variance * mean));
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
    int n, dead = 0, peasants = rp;
    int satiated = money / maintenance_cost(NULL);
    int max_survive = (maxp < satiated) ? maxp : satiated;

    if (peasants > 0 && rule > 0 && peasants < max_survive) {
        int luck = 0;
        double fraction = peasants * peasant_growth_factor();
        int births = ROUND_BIRTHS(fraction);

        attrib *a = a_find(r->attribs, &at_peasantluck);

        if (a != NULL) {
            luck = a->data.i * 1000;
        }

        luck = peasant_luck_effect(peasants, luck, maxp, .5);
        if (luck > 0) {
            ADDMSG(&r->msgs, msg_message("peasantluck_success", "births", luck));
            births += luck;
        }
        if (peasants + births > max_survive) {
            /* if we cannot afford to breed, we don't */
            births = max_survive - peasants;
        }
        peasants += births;
    }

    /* Alle werden satt, oder halt soviele fuer die es auch Geld gibt */

    if (satiated > peasants) satiated = peasants;
    rsetmoney(r, money - satiated * maintenance_cost(NULL));

    /* Von denjenigen, die nicht satt geworden sind, verhungert der
     * Grossteil. dead kann nie groesser als rpeasants(r) - satiated werden,
     * so dass rpeasants(r) >= 0 bleiben muss. */

     /* Es verhungert maximal die unterernaehrten Bevoelkerung. */

    n = peasants - satiated;
    if (n > rp) n = rp;
    dead += (int)(0.5 + n * PEASANT_STARVATION_CHANCE);

    if (dead > 0) {
        ADDMSG(&r->msgs, msg_message("phunger", "dead", dead));
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
        if (!m) {
            m = calloc(1, sizeof(migration));
            if (!m) abort();
        }
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
         * wer fragt das? Die Baumwanderung war abhaengig von der
         * Auswertungsreihenfolge der regionen,
         * das hatte ich geaendert. jemand hat es wieder geloescht, toll.
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
    maxhorses = region_production(r) / 10;
    horses = rhorses(r);
    if (horses > 0) {
        if (maxhorses > 0) {
            double growth =
                ((HORSEGROWTH * 100.0 * ((double)maxhorses -
                    horses))) / (double)maxhorses;

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
     * die dann erst in die Berechnung der Nachbarstruktur einfliesst.
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
                 * wenn nicht, muessen wir sie suchen.
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

static int cap_int(int i, int imin, int imax) {
    if (i > imin) {
        return ((i < imax) ? i : imax);
    }
    return imin;
}

static bool
increased_growth(const region* r, const struct race *rc_elf) {
    const unit* u;
    for (u = r->units; u; u = u->next) {
        if (u_race(u) != rc_elf) {
            return false;
        }
    }
    return true;
}

static void
growing_trees(region * r, const season_t current_season, const season_t last_weeks_season, int rules)
{
    int grownup_trees, i, seeds;
    double seedchance = config_get_flt("rules.treeseeds.chance", 0.005F);

    if (current_season == SEASON_SUMMER || current_season == SEASON_AUTUMN) {
        const struct race* rc_elf = get_race(RC_ELF);
        int mp, elves = count_race(r, rc_elf);
        direction_t d;

        if (last_weeks_season == SEASON_SPRING) {
            attrib * a = a_find(r->attribs, &at_germs);
            /* ungekeimte Samen bleiben erhalten, Sproesslinge wachsen */
            if (a) {
                int sprout = rtrees(r, 1);
                if (sprout > a->data.sa[1]) sprout = a->data.sa[1];
                /* aus dem gesamt Sproesslingepool abziehen */
                rsettrees(r, 1, rtrees(r, 1) - sprout);
                /* zu den Baeumen hinzufuegen */
                rsettrees(r, 2, rtrees(r, 2) + sprout);

                a_removeall(&r->attribs, &at_germs);
            }
        }

        mp = max_production(r);
        if (mp <= 0)
            return;

        /* Grundchance 1.0% */
        /* Jeder Elf in der Region erhoeht die Chance marginal */
        mp = mp / 8;
        if (elves > mp) elves = mp;
        if (elves) {
            seedchance += 1.0 - pow(0.99999, elves / 2.0);
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
                if (rules > 2) {
                    if (increased_growth(r, rc_elf)) {
                        /* in empty regions, plant twice the seeds */
                        seeds += seeds;
                    }
                }
                seeds += rtrees(r, 0);
                rsettrees(r, 0, seeds);
            }
        }

        /* Baeume breiten sich in Nachbarregionen aus. */

        /* Gesamtzahl der Samen:
         * bis zu 6% (FORESTGROWTH*3) der Baeume samen in die Nachbarregionen */
        if (seedchance > 0) {
            seeds = (rtrees(r, 2) * FORESTGROWTH * 3) / 1000000;
            for (d = 0; d != MAXDIRECTIONS; ++d) {
                region *r2 = rconnect(r, d);
                if (r2 && r2->land && r2->terrain->size) {
                    /* Eine Landregion, wir versuchen Samen zu verteilen:
                     * Die Chance, das Samen ein Stueck Boden finden, in dem sie
                     * keimen koennen, haengt von der Bewuchsdichte und der
                     * verfuegbaren Flaeche ab. In Gletschern gibt es weniger
                     * Moeglichkeiten als in Ebenen. */
                    int sprout = 0;
                    seedchance = (1000.0 * region_production(r2)) / r2->terrain->size;
                    for (i = 0; i < seeds / MAXDIRECTIONS; i++) {
                        if (rng_int() % 10000 < seedchance)
                            ++sprout;
                    }
                    rsettrees(r2, 0, rtrees(r2, 0) + sprout);
                }
            }
        }
    }
    else if (current_season == SEASON_SPRING) {
        /* in at_germs merken uns die Zahl der Samen und Sproesslinge, die
         * dieses Jahr aelter werden duerfen, damit nicht ein Same im selben
         * Zyklus zum Baum werden kann */
        attrib * a = a_find(r->attribs, &at_germs);
        int sprout = rtrees(r, 1);
        if (!a) {
            a = a_add(&r->attribs, a_new(&at_germs));
            a->data.sa[0] = (short)cap_int((8 + rtrees(r, 0)) / 9, 0, SHRT_MAX);
            a->data.sa[1] = (short)cap_int((8 + rtrees(r, 1)) / 9, 0, SHRT_MAX);
        }
        else if (a->data.sa[0] < 0 || a->data.sa[1] < 0) {
            a->data.sa[0] = (short)cap_int(a->data.sa[0], 0, SHRT_MAX);
            a->data.sa[1] = (short)cap_int(a->data.sa[1], 0, SHRT_MAX);
        }

        /* Baumwachstum */
        if (sprout > a->data.sa[1]) sprout = a->data.sa[1];
        grownup_trees = sprout;
        /* aus dem gesamt Sproesslingepool abziehen */
        rsettrees(r, 1, rtrees(r, 1) - grownup_trees);
        /* zu den Baeumen hinzufuegen */
        rsettrees(r, 2, rtrees(r, 2) + grownup_trees);

        /* Samenwachstum, wenn noch Platz f�r Spr��linge ist: */
        i = region_maxworkers(r, max_production(r));
        if (i > 0) {
            seeds = rtrees(r, 0);
            if (seeds > a->data.sa[0]) seeds = a->data.sa[0];
            /* aus dem gesamt Samenpool abziehen */
            rsettrees(r, 0, rtrees(r, 0) - seeds);
            /* zu den Sproesslinge hinzufuegen */
            rsettrees(r, 1, rtrees(r, 1) + seeds);
        }
    }
}

static void
growing_herbs(region * r, const int current_season)
{
    /* Jetzt die Kraeutervermehrung. Vermehrt wird logistisch:
     *
     * Jedes Kraut hat eine Wahrscheinlichkeit von (100-(vorhandene
     * Kraeuter))% sich zu vermehren. */
    if (current_season != SEASON_WINTER) {
        int i, herbs = rherbs(r) + 1;
        for (i = herbs; i > 0; --i) {
            if (rng_int() % 100 < (100 - herbs)) {
                ++herbs;
            }
        }
        rsetherbs(r, herbs);
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
            if (rp < 0) rp = 0;
            rsetpeasants(r, rp);
            r->land->newpeasants = 0;
        }
        /* Generate some (0-6 depending on the income) peasants out of nothing */
        /* if less than `repopulate` are in the region and there is space and no monster or demon units in the region */
        if (repopulate) {
            int peasants = rpeasants(r);
            bool mourn = is_mourning(r, turn);
            int income = peasant_wage(r, mourn) - maintenance_cost(NULL) + 1;
            if (income >= 0 && r->land && (peasants < repopulate) && region_production(r) >(peasants + 30) * 2) {
                int badunit = 0;
                unit *u;
                for (u = r->units; u; u = u->next) {
                    if (!playerrace(u_race(u)) || u_race(u) == get_race(RC_DAEMON)) {
                        badunit = 1;
                        break;
                    }
                }
                if (badunit == 0) {
                    peasants += (int)(rng_double() * income);
                    rsetpeasants(r, peasants);
                }
            }
        }
    }
}

void nmr_warnings(void)
{
    faction *f, *fa;
#define HELP_NMR (HELP_GUARD|HELP_MONEY)
    for (f = factions; f; f = f->next) {
        if (!fval(f, FFL_NOIDLEOUT|FFL_PAUSED) && turn > f->lastorders) {
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
                    else if (alliedfaction(f, fa, HELP_NMR) && alliedfaction(fa, f, HELP_NMR)) {
                        warn = 1;
                    }
                    if (warn) {
                        if (msg == NULL) {
                            msg = msg_message("warn_dropout", "faction turns", f,
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

static void cb_increase_demand(struct demand *dmd, int n, void *data)
{
    region *r = (region * )data;
    const struct building_type *bt_harbour = NULL;
    int i;
    for (i = 0; i != n; ++i) {
        if (dmd[i].value > 0 && dmd[i].value < MAXDEMAND) {
            float rise = DMRISE;
            if (r->buildings) {
                if (!bt_harbour) bt_harbour = bt_find("harbour");
                if (buildingtype_exists(r, bt_harbour, true)) {
                    rise = DMRISEHAFEN;
                }
            }
            if (rng_double() <= rise)
                ++dmd[i].value;
        }
    }
}

void demographics_week(int week)
{
    region *r;
    int plant_rules = config_get_int("rules.grow.formula", 3);
    int horse_rules = config_get_int("rules.horses.growth", 1);
    int peasant_rules = config_get_int("rules.peasants.growth", 1);
    season_t current_season = calendar_season(week);
    season_t last_weeks_season = calendar_season(week + weeks_per_month * months_per_year - 1);

    for (r = regions; r; r = r->next) {
        /** Ageing of regions starts when they are first discovered.
         * This should prevent monsters from being created there. */
        if (r->age>0 || r->units || r->attribs) {
            ++r->age; /* also oceans. no idea why we didn't always do that */
        }
        live(r);

        if (!fval(r->terrain, SEA_REGION)) {
            /* die Nachfrage nach Produkten steigt. */
            if (r->land) {
                r_foreach_demand(r, cb_increase_demand, r);
                /* Seuchen erst nachdem die Bauern sich vermehrt haben
                 * und gewandert sind */

                peasant_migration(r);
                peasants(r, peasant_rules);

                if (r->age > 20) {
                    double mwp = fmax(region_production(r), 1);
                    bool mourn = is_mourning(r, week);
                    int p_wage = peasant_wage(r, mourn);
                    double prob =
                        pow(rpeasants(r) / (mwp * p_wage * 0.13), 4.0)
                        * PLAGUE_CHANCE;

                    if (rng_double() < prob) {
                        plagues(r);
                    }
                }
                if (horse_rules > 0) {
                    horses(r);
                }
                if (plant_rules==1) { /* E3 */
                    growing_trees_e3(r, current_season, last_weeks_season);
                }
                else if (plant_rules) { /* E2 */
                    growing_trees(r, current_season, last_weeks_season, plant_rules);
                    growing_herbs(r, current_season);
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

void demographics(void)
{
    demographics_week(turn);
}

int leave_cmd(unit * u, struct order *ord)
{
    region *r = u->region;

    if (fval(u, UFL_ENTER)) {
        /* if we just entered this round, then we don't leave again */
        return 0;
    }

    if (fval(r->terrain, SEA_REGION) && u->ship) {
        if (!fval(u_race(u), RCF_SWIM|RCF_FLY)) {
            cmistake(u, ord, 11, MSG_MOVE);
            return 0;
        }
        if (has_horses(u)) {
            cmistake(u, ord, 231, MSG_MOVE);
            return 0;
        }
    }
    leave(u, true);
    return 0;
}

void transfer_faction(faction *fsrc, faction *fdst) {
    unit *u;
    skill_t sk;
    int hmax, hnow;
    int skill_count[MAXSKILLS];
    int skill_limit[MAXSKILLS];

    assert(fsrc != fdst);

    for (sk = 0; sk != MAXSKILLS; ++sk) {
        skill_limit[sk] = faction_skill_limit(fdst, sk);
    }
    memset(skill_count, 0, sizeof(skill_count));

    for (u = fdst->units; u != NULL; u = u->nextF) {
        if (u->skills) {
            ptrdiff_t s, len = arrlen(u->skills);
            for (s = 0; s != len; ++s) {
                skill_t sk = (skill_t)u->skills[s].id;
                skill_count[sk] += u->number;
            }
        }
    }

    hnow = countheroes(fdst);
    hmax = max_heroes(fdst->num_people);
    u = fsrc->units;
    while (u) {
        unit *unext = u->nextF;

        if (u_race(u) == fdst->race) {
            if (u->flags & UFL_HERO) {
                if (u->number + hnow > hmax) {
                    u->flags &= ~UFL_HERO;
                }
                else {
                    hnow += u->number;
                }
            }
            if (give_unit_allowed(u) == 0) {
                if (fdst->magiegebiet != M_NONE) {
                    struct sc_mage *m = get_mage(u);
                    if (m && mage_get_type(m) != fdst->magiegebiet) {
                        u = unext;
                        continue;
                    }
                }

                if (u->skills) {
                    ptrdiff_t s, len = arrlen(u->skills);
                    for (s = 0; s != len; ++s) {
                        const skill *sv = u->skills + s;
                        skill_t sk = (skill_t)sv->id;

                        if (skill_count[sk] + u->number > skill_limit[sk]) {
                            break;
                        }
                    }
                    if (s != len) {
                        u = unext;
                        continue;
                    }
                }
                ADDMSG(&fdst->msgs, msg_message("transfer_unit", "unit", u));
                u_setfaction(u, fdst);
            }
        }
        u = unext;
    }
}

int quit_cmd(unit * u, struct order *ord)
{
    char token[128];
    faction *f = u->faction;
    const char *passwd;

    init_order(ord, NULL);
    passwd = gettoken(token, sizeof(token));
    if (checkpasswd(f, (const char *)passwd)) {
        int flags = FFL_QUIT;
        if (rule_transfermen()) {
            param_t p;
            const char * s = gettoken(token, sizeof(token));
            if (s) {
                p = get_param(s, f->locale);
                if (p != P_FACTION) {
                    log_error("faction %s: QUIT FACTION syntax error.", factionname(f));
                    cmistake(u, ord, 209, MSG_EVENT);
                    flags = 0;
                } else {
#ifdef QUIT_WITH_TRANSFER
                    faction *f2 = getfaction();
                    if (f2 == NULL || f2 == u->faction) {
                        cmistake(u, ord, 66, MSG_EVENT);
                        flags = 0;
                    }
                    else if (f->race != f2->race) {
                        cmistake(u, ord, 281, MSG_EVENT);
                        flags = 0;
                    }
                    else {
                        unit *u2;
                        for (u2 = u->region->units; u2; u2 = u2->next) {
                            if (u2->faction == f2) {
                                if (ucontact(u2, u)) {
                                    transfer_faction(u->faction, u2->faction);
                                    break;
                                }
                            }
                        }
                        if (u2 == NULL) {
                            /* no target unit found */
                            cmistake(u, ord, 40, MSG_EVENT);
                            flags = 0;
                        }
                    }
#else
                    log_error("faction %s: QUIT FACTION is disabled.", factionname(f));
                    flags = 0;
#endif
                }
            }
        }
        f->flags |= flags;
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
    if (sh == NULL || sh->number < 1 || sh->region != r) {
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
        int mweight = ship_capacity(sh);
        int mcabins = ship_cabins(sh);

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
    const struct race* rc = u_race(u);

    /* Schwimmer koennen keine Gebaeude betreten, ausser diese sind
     * auf dem Ozean */
    if (fval(rc, RCF_SWIM|RCF_WALK|RCF_FLY) == RCF_SWIM) {
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

        if (IS_PAUSED(u->faction)) continue;

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

        if (IS_PAUSED(u->faction)) {
            uptr = &u->next;
            continue;
        }

        while (*ordp) {
            order *ord = *ordp;
            if (getkeyword(ord) == K_ENTER) {
                char token[128];
                param_t p;
                int id;
                unit *ulast = NULL;
                const char * s;

                init_order(ord, NULL);
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

int newbies[MAXNEWPLAYERS];
int dropouts[2];

bool nmr_death(const faction * f, int turn, int timeout)
{
    if (faction_age(f) >= timeout && turn - f->lastorders >= timeout) {
        static bool rule_destroy;
        static int config;
        
        if (config_changed(&config)) {
            rule_destroy = config_get_int("rules.nmr.destroy", 0) != 0;
        }
        if (rule_destroy) {
            unit *u;
            for (u = f->units; u; u = u->nextF) {
                if (u->building && building_owner(u->building) == u) {
                    remove_building(&u->region->buildings, u->building);
                }
            }
        }
        return true;
    }
    return false;
}

static void remove_idle_players(void)
{
    faction **fp;
    int timeout = NMRTimeout();
    log_info(" - beseitige Spieler, die sich zu lange nicht mehr gemeldet haben...");

    for (fp = &factions; *fp;) {
        faction *f = *fp;

        if (timeout > 0 && nmr_death(f, turn, timeout) && !fval(f, FFL_PAUSED)) {
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

    if (RemoveNMRNewbie()) {
        for (fp = &factions; *fp;) {
            faction* f = *fp;
            if (!IS_MONSTERS(f)) {
                if (!fval(f, FFL_PAUSED | FFL_NOIDLEOUT)) {
                    int age = faction_age(f);
                    if (age >= 0 && age < MAXNEWPLAYERS) {
                        ++newbies[age];
                    }
                    if (age == 2 || age == 3) {
                        if (f->lastorders == turn - 2) {
                            ++dropouts[age - 2];
                            destroyfaction(fp);
                            continue;
                        }
                    }
                }
            }
            fp = &f->next;
        }
    }
}

void quit(void)
{
    faction **fptr = &factions;
    while (*fptr) {
        faction *f = *fptr;
        if ((f->flags & FFL_QUIT) && !IS_PAUSED(f)) {
            destroyfaction(fptr);
        }
        else {
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
    struct ally **sfp;
    faction *f;
    int keyword, not_kw;
    const char *s;
    int sf_status;
   
    init_order(ord, NULL);
    f = getfaction();

    if (f == NULL || IS_MONSTERS(f)) {
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
        keyword = get_param(s, u->faction->locale);
    }

    sfp = &u->faction->allies;
    if (fval(u, UFL_GROUP)) {
        group *g = get_group(u);
        if (g) {
            sfp = &g->allies;
        }
    }

    not_kw = getparam(u->faction->locale);        /* HELFE partei [modus] NICHT */

    sf_status = ally_get(*sfp, f);
    switch (keyword) {
    case P_NOT:
        sf_status = 0;
        break;

    case NOPARAM:
        cmistake(u, ord, 137, MSG_EVENT);
        return 0;

    case P_ANY:
        sf_status = (not_kw == P_NOT) ? 0 : HELP_ALL;
        break;

    case P_TRAVEL:
        if (not_kw == P_NOT) {
            sf_status = sf_status & (HELP_ALL - HELP_TRAVEL);
        }
        else {
            sf_status |= HELP_TRAVEL;
        }
        break;

    case P_GIVE:
        if (not_kw == P_NOT)
            sf_status &= (HELP_ALL - HELP_GIVE);
        else
            sf_status |= HELP_GIVE;
        break;

    case P_MONEY:
        if (not_kw == P_NOT)
            sf_status &= (HELP_ALL - HELP_MONEY);
        else
            sf_status |= HELP_MONEY;
        break;

    case P_FIGHT:
        if (not_kw == P_NOT)
            sf_status &= (HELP_ALL - HELP_FIGHT);
        else
            sf_status |= HELP_FIGHT;
        break;

    case P_FACTIONSTEALTH:
        if (not_kw == P_NOT)
            sf_status &= (HELP_ALL - HELP_FSTEALTH);
        else
            sf_status |= HELP_FSTEALTH;
        break;

    case P_GUARD:
        if (not_kw == P_NOT)
            sf_status &= (HELP_ALL - HELP_GUARD);
        else
            sf_status |= HELP_GUARD;
        break;
    }

    sf_status &= HelpMask();
    ally_set(sfp, f, sf_status);

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
        if (in == NULL) {
            in = calloc(1, sizeof(local_names));
            if (!in) abort();
        }
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
        for (in = pnames; in && in->lang != lang; in = in->next);
        if (!in) return 0;
    }

    init_order(ord, NULL);
    s = gettoken(token, sizeof(token));

    if (!s || !*s) {
        group *g = get_group(u);
        if (g) {
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
        group *g = get_group(u);
        if (g) {
            ap = &g->attribs;
        } 
        else {
            ap = &u->faction->attribs;
        }
        set_prefix(ap, race_prefixes[var.i]);
    }
    return 0;
}

int display_cmd(unit * u, struct order *ord)
{
    char token[128];
    char **s = NULL;
    char *str;
    region *r = u->region;

    init_order(ord, NULL);

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
        str = getstrtoken();
        if (str) {
            utf8_trim(str);
            utf8_clean(str);
        }
        unit_setinfo(u, str);
        break;

    case P_PRIVAT:
        str = getstrtoken();
        if (str) {
            utf8_trim(str);
            utf8_clean(str);
        }
        usetprivate(u, str);
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
            if (utf8_trim(sdup) != 0) {
                log_info("trimming info: %s", s2);
            }
            utf8_clean(str);
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
    if (utf8_trim(name) != 0) {
        log_info("trimming name: %s", s2);
    }
    utf8_clean(name);

    free(*s);
    *s = str_strdup(name);
    return 0;
}

static bool can_rename_building(unit *u, building *b, order *ord, bool foreign) {
    unit *owner = b ? building_owner(b) : NULL;

    if (!b) {
        cmistake(u, ord, u->building ? 6 : 145, MSG_EVENT);
        return false;
    }

    if (!fval(b->type, BTF_NAMECHANGE) && renamed_building(b)) {
        cmistake(u, ord, 278, MSG_EVENT);
        return false;
    }

    if (foreign) {
        message *seen;
        if (renamed_building(b)) {
            cmistake(u, ord, 246, MSG_EVENT);
            return false;
        }

        seen = msg_message("renamed_building_seen",
            "building renamer region", b, u, u->region);
        add_message(&u->faction->msgs, seen);

        if (owner && owner->faction != u->faction) {
            if (cansee(owner->faction, u->region, u, 0)) {
                add_message(&owner->faction->msgs, seen);
            }
            else {
                ADDMSG(&owner->faction->msgs,
                    msg_message("renamed_building_notseen",
                        "building region", b, u->region));
            }
        }
        msg_release(seen);
    }
    else if (owner && owner->faction != u->faction) {
        cmistake(u, ord, 148, MSG_PRODUCE);
        return false;
    }
    return true;
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
    const char* name = NULL;

    init_order(ord, u->faction->locale);
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
            if (!b) {
                cmistake(u, ord, 31, MSG_EVENT);
                break;
            }
        }
        if (can_rename_building(u, b, ord, foreign)) {
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
            if (faction_age(f) < 10) {
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

                    sdname = param_name(P_SHIP, lang);
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
            unit *u2 = NULL;

            getunit(r, u->faction, &u2);
            if (!u2 || u2->region != r || !cansee(u->faction, r, u2, 0)) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, ord,
                    "feedback_unit_not_found", NULL));
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
        group *g = get_group(u);
        if (g) {
            name = getstrtoken();
            if (name == NULL) {
                cmistake(u, ord, 84, MSG_EVENT);
                return 0;
            }
            if (find_groupbyname(u->faction, name)) {
                cmistake(u, ord, 332, MSG_EVENT);
            }
            else {
                s = &g->name;
            }
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
        if (name == NULL) {
            name = getstrtoken();
        }
        if (name == NULL) {
            cmistake(u, ord, 84, MSG_EVENT);
        }
        else {
            rename_cmd(u, ord, s, name);
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
            msg_feedback(u, ord, "feedback_unit_not_found", NULL));
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

    init_order(ord, NULL);
    s = gettoken(token, sizeof(token));

    /* Falls kein Parameter, ist das eine Einheitsnummer;
     * das Fuellwort "AN" muss wegfallen, da gueltige Nummer! */

    do {
        cont = 0;
        switch (findparam_ex(s, u->faction->locale)) {
        case P_REGION:
            /* koennen alle Einheiten in der Region sehen */
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
            n = getid();

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
                    "feedback_unit_not_found", NULL));
                return 0;
            }

            s = getstrtoken();
            if (!s || !s[0]) {
                cmistake(u, ord, 30, MSG_MESSAGE);
                break;
            }
            else {
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

    init_order(ord, NULL);
    s = getstrtoken();
    faction_setbanner(u->faction, s);
    ADDMSG(&u->faction->msgs, msg_message("changebanner", "value", s));

    return 0;
}

int email_cmd(unit * u, struct order *ord)
{
    const char *s;

    init_order(ord, NULL);
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

bool password_wellformed(const char *password)
{
    unsigned char *c = (unsigned char *)password;
    int i;
    if (!password || password[0]=='\0') {
        return false;
    }
    for (i = 0; c[i] && i != PASSWORD_MAXSIZE; ++i) {
        if (!isalnum(c[i])) {
            return false;
        }
    }
    return true;
}

int password_cmd(unit * u, struct order *ord)
{
    char pwbuf[PASSWORD_MAXSIZE + 1];
    const char *s;

    init_order(ord, NULL);
    pwbuf[PASSWORD_MAXSIZE] = '\n';
    s = gettoken(pwbuf, sizeof(pwbuf));
    if (pwbuf[PASSWORD_MAXSIZE] == '\0') {
        cmistake(u, ord, 321, MSG_EVENT);
        pwbuf[PASSWORD_MAXSIZE - 1] = '\0';
    }

    if (!s || !password_wellformed(s)) {
        if (s) {
            cmistake(u, ord, 283, MSG_EVENT);
        }
        password_generate(pwbuf, PASSWORD_MAXSIZE);
    }
    faction_setpassword(u->faction, password_hash(pwbuf, PASSWORD_DEFAULT));
    ADDMSG(&u->faction->msgs, msg_message("changepasswd", "value", pwbuf));
    u->faction->flags |= FFL_PWMSG;
    return 0;
}

int send_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *s;
    int option;

    init_order(ord, NULL);
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
    sbstring sbs;

    sbs_init(&sbs, buf, sizeof(buf));
    report_raceinfo(rc, f->locale, &sbs);
    addmessage(0, f, buf, MSG_EVENT, ML_IMPORTANT);
}

static void reshow_other(unit * u, struct order *ord, const char *s) {
    int err = 21;
    bool found = false;

    if (s) {
        const spell *sp = NULL;
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
                if (level > 0 && 2 * level <= effskill(u, SK_ALCHEMY, NULL)) {
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

    if (max_heroes(u->faction->num_people) < countheroes(u->faction) + u->number) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "heroes_maxed", "max count",
                max_heroes(u->faction->num_people), countheroes(u->faction)));
        return 0;
    }
    if (!valid_race(u->faction, u_race(u))) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "heroes_race", "race",
            u_race(u)));
        return 0;
    }
    people = u->faction->num_people * u->number;
    money = get_pooled(u, rsilver, GET_DEFAULT, people);

    if (people > money) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "heroes_cost", "cost have", people, money));
        return 0;
    }
    use_pooled(u, rsilver, GET_DEFAULT, people);
    fset(u, UFL_HERO);
    ADDMSG(&u->faction->msgs, msg_message("hero_promotion", "unit cost",
        u, people));
    return 0;
}

int group_cmd(unit * u, struct order *ord)
{
    init_order(ord, NULL);
    join_group(u, getstrtoken());
    return 0;
}

int origin_cmd(unit * u, struct order *ord)
{
    int px, py;

    init_order(ord, NULL);

    px = getint();
    py = getint();

    faction_setorigin(u->faction, getplaneid(u->region), px, py);
    return 0;
}

int guard_off_cmd(unit * u, struct order *ord)
{
    assert(getkeyword(ord) == K_GUARD);
    init_order(ord, NULL);

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

    init_order(ord, NULL);
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

    init_order(ord, NULL);
    s = gettoken(token, sizeof(token));
    switch (get_param(s, u->faction->locale)) {
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
    spell *sp = NULL;

    init_order(ord, NULL);
    s = gettoken(token, sizeof(token));

    /* KAMPFZAUBER [NICHT] loescht alle gesetzten Kampfzauber */
    if (!s || *s == 0 || isparam(s, u->faction->locale, P_NOT)) {
        unset_combatspell(u, 0);
        return 0;
    }

    /* Optional: STUFE n */
    if (isparam(s, u->faction->locale, P_LEVEL)) {
        /* Merken, setzen kommt erst spaeter */
        level = getuint();
        s = gettoken(token, sizeof(token));
    }

    sp = unit_getspell(u, s, u->faction->locale);
    if (!sp) {
        cmistake(u, ord, 173, MSG_MAGIC);
        return 0;
    }

    s = gettoken(token, sizeof(token));

    if (isparam(s, u->faction->locale, P_NOT)) {
        /* KAMPFZAUBER "<Spruchname>" NICHT  loescht diesen speziellen
         * Kampfzauber */
        unset_combatspell(u, sp);
        return 0;
    }
    else {
        /* KAMPFZAUBER "<Spruchname>"  setzt diesen Kampfzauber */
        /* knowsspell prueft auf ist_magier, ist_spruch, kennt_spruch */
        if (!knowsspell(u->region, u, sp)) {
            /* Fehler 'Spell not found' */
            cmistake(u, ord, 173, MSG_MAGIC);
        }
        else if (!u_hasspell(u, sp)) {
            /* Diesen Zauber kennt die Einheit nicht */
            cmistake(u, ord, 169, MSG_MAGIC);
        }
        else if (!(sp->sptyp & ISCOMBATSPELL)) {
            /* Diesen Kampfzauber gibt es nicht */
            cmistake(u, ord, 171, MSG_MAGIC);
        }
        else {
            set_combatspell(u, sp, ord, level);
        }
    }

    return 0;
}

int guard_on_cmd(unit * u, struct order *ord)
{
    assert(getkeyword(ord) == K_GUARD);
    assert(u);
    assert(u->faction);

    init_order(ord, NULL);

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

        if (sh->number > 0) {
            if (!sh->type->construction || sh->size >= sh->type->construction->maxsize) {
                unit *cap = ship_owner(sh);
                if (fval(r->terrain, SEA_REGION)) {
                    if (!ship_crewed(sh, cap)) {
                        /* ship is at sea, but not enough people to control it */
                        double dmg = config_get_flt("rules.ship.damage.nocrewocean", 0.3);
                        damage_ship(sh, dmg);
                    }
                }
                else if (!cap) {
                    /* any ship lying around without an owner slowly rots */
                    double dmg = config_get_flt("rules.ship.damage.nocrew", 0.05);
                    damage_ship(sh, dmg);
                }
            }
            if (ship_damage_percent(sh) >= 100) {
                sink_ship(sh);
                remove_ship(shp, sh);
            }
        }
        else {
            remove_ship(shp, sh);
        }
        if (*shp == sh)
            shp = &sh->next;
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
                }
                if (unicorns) {
                    ADDMSG(&u->faction->msgs, msg_message("scunicorn",
                        "unit amount rtype",
                        u, unicorns, rtype));
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
                int sk = effskill(mage, SK_MAGIC, NULL);
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
                int sk = effskill(mage, SK_MAGIC, NULL);
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

    /* Factions */
    for (f = factions; f; f = f->next) {
        a_age(&f->attribs, f);
        handle_event(f->attribs, "timer", f);
    }

    /* Regionen */
    for (r = regions; r; r = r->next) {
        building **bp;
        unit **up = &r->units;
        ship **sp;

        while (*up) {
            unit *u = *up;
            /* IUW: age_unit() kann u loeschen, u->next ist dann
             * undefiniert, also muessen wir hier schon das naechste
             * Element bestimmen */
            age_unit(up);
            if (*up == u) {
                up = &u->next;
            }
        }

        age_region(r);

        /* Schiffe */
        for (sp = &r->ships; *sp;) {
            ship *s = *sp;
            a_age(&s->attribs, s);
            if (s == *sp)
                handle_event(s->attribs, "timer", s);
            if (s == *sp) /*-V581 */
                sp = &(*sp)->next;
        }

        /* Gebaeude */
        for (bp = &r->buildings; *bp;) {
            building *b = *bp;
            age_building(b);
            if (b == *bp)
                bp = &b->next;
        }

        if (rule_region_owners()) {
            update_region_owners(r);
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

static void maketemp_cmd(unit *u, order **olist) 
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
        
        init_order(makeord, NULL);
        alias = getid();
        s = gettoken(token, sizeof(token));
        if (s && s[0] == '\0') {
            /* empty name? => generate one */
            s = NULL;
        }
        u2 = create_unit(u->region, u->faction, 0, u->faction->race, alias, s, u);
        fset(u2, UFL_ISNEW);
        usetalias(u2, alias);
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
                unit_addorder(u2, deford);
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

            if (IS_PAUSED(u->faction)) continue;

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

static int callback_use(unit *u, const item_type *itype, int amount, struct order *ord)
{
    int len;
    char fname[64];

    len = snprintf(fname, sizeof(fname), "use_%s", itype->rtype->_name);
    if (len > 0 && (size_t)len < sizeof(fname)) {
        int(*callout)(unit *, const item_type *, int, struct order *);

        /* check if we have a register_item_use function */
        callout = (int(*)(unit *, const item_type *, int, struct order *))get_function(fname);
        if (callout) {
            return callout(u, itype, amount, ord);
        }
    }
    /* if the item is a potion, try use_potion,
     * the generic function for potions that add an effect:
     */
    if (itype->flags & ITF_POTION) {
        return use_potion(u, itype, amount, ord);
    }

    /* finally, check if we have a matching lua function */
    if (callbacks.use_item) {
        return callbacks.use_item(u, itype, amount, ord);
    }

    return EUNUSABLE;
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
        int result = callback_use(u, itype, amount, ord);
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
            /* bonus zuruecksetzen */
            curse *c = get_curse(r->attribs, &ct_healing);
            if (c != NULL) {
                healingcurse = curse_geteffect(c);
            }
        }
        for (u = r->units; u; u = u->next) {
            int umhp = unit_max_hp(u) * u->number;
            double p = 1.0;

            /* hp ueber Maximum bauen sich ab. Wird zb durch Elixier der Macht
             * oder veraendertes Ausdauertalent verursacht */
            if (u->hp > umhp) {
                int diff = u->hp - umhp;
                u->hp -= (int)ceil(diff / 2.0);
                if (u->hp < umhp) {
                    u->hp = umhp;
                }
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
                /* pro punkt 5% hoeher */
                p *= (1.0 + healingcurse * 0.05);

                maxheal = p * maxheal;
                addhp = (int)maxheal;
                maxheal -= addhp;
                if (maxheal > 0.0 && chance(maxheal))
                    ++addhp;

                /* Aufaddieren der geheilten HP. */
                if (umhp > u->hp + addhp) umhp = u->hp + addhp;
                u->hp = umhp;

                /* soll man an negativer regeneration sterben koennen? */
                assert(u->hp > 0);
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
            struct sc_mage * mage = get_mage(u);
            if (mage) {
                int level = effskill(u, SK_MAGIC, NULL);
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
        ptrdiff_t qi, ql = arrlen(src->spells);

        for (qi = 0; qi != ql; ++qi) {
            spellbook_entry *sbe = (spellbook_entry *)src->spells + qi;
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
        ptrdiff_t qi, ql = arrlen(book->spells);

        for (qi = 0; qi != ql; ++qi) {
            spellbook_entry *sbe = (spellbook_entry *)book->spells + qi;
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
        if (f->magiegebiet != M_NONE && !IS_MONSTERS(f)) {
            unit *mages[MAXMAGES];
            int i;
            int maxlevel = faction_getmages(f, mages, MAXMAGES);
            struct spellbook *fsb;

            if (maxlevel && FactionSpells()) {
                spellbook * book = get_spellbook(magic_school[f->magiegebiet]);
                if (!f->spellbook) {
                    f->spellbook = create_spellbook(NULL);
                }
                copy_spells(book, f->spellbook, maxlevel);
                if (maxlevel > f->max_spelllevel) {
                    spellbook * common_spells = get_spellbook(magic_school[M_COMMON]);
                    pick_random_spells(f, maxlevel, common_spells, COMMONSPELLS);
                }
            }
            fsb = faction_get_spellbook(f);
            show_new_spells(f, maxlevel, fsb);
            for (i = 0; i != MAXMAGES && mages[i]; ++i) {
                unit * u = mages[i];
                spellbook *sb = unit_get_spellbook(u);
                if (sb != fsb) {
                    int level = effskill(u, SK_MAGIC, NULL);
                    show_new_spells(f, level, sb);
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

    init_order(ord, NULL);

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
        if (err < 0) {
            cmistake(u, ord, -err, MSG_PRODUCE);
        }
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

        init_order(ord, NULL);
        p = getparam(u->faction->locale);
        id = getid();
        if (p == P_NOT) {
            unit *owner = building_owner(u->building);
            /* If the unit is not the owner of the building: error */
            if (owner->no != u->no) {
                /* The building is not ours error */
                cmistake(u, ord, 5, MSG_EVENT);
            }
            else {
                /* If no building id is given or it is the id of our building, just set the do-not-pay flag */
                if (id == 0 || id == u->building->no) {
                    u->building->flags |= BLD_DONTPAY;
                }
                else {
                    /* Find the building that matches to the given id*/
                    building *b = findbuilding(id);
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
                            cmistake(u, ord, 5, MSG_EVENT);
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

int claim_cmd(unit * u, struct order *ord)
{
    char token[128];
    const char *t;
    int n = 1;
    const item_type *itype = NULL;

    init_order(ord, NULL);

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
    if (!proc) abort();
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

bool long_order_allowed(const unit *u, bool flags_only)
{
    const region *r = u->region;

    if (IS_PAUSED(u->faction)) return false;
    if (fval(u, UFL_LONGACTION)) {
        /* this message was already given in update_long_orders();
        cmistake(u, ord, 52, MSG_PRODUCE);
        */
        return false;
    }
    if (flags_only) return true;
    if (fval(r->terrain, SEA_REGION) && !(u_race(u)->flags & RCF_SWIM)) {
        if (u_race(u) != get_race(RC_AQUARIAN)) {
            /* error message disabled by popular demand */
            return false;
        }
    }
    return true;
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
            if (proc->name) {
                log_debug(" - %s", proc->name);
            }
            proc = proc->next;
        }

        while (pglobal && pglobal->priority == prio && pglobal->type == PR_GLOBAL) {
            pglobal->data.global.process();
            pglobal = pglobal->next;
        }
        if (pglobal == NULL || pglobal->priority != prio) {
            continue;
        }

        for (r = regions; r; r = r->next) {
            unit *u;
            processor *pregion = pglobal;

            while (pregion && pregion->priority == prio
                && pregion->type == PR_REGION_PRE) {
                pregion->data.per_region.process(r);
                pregion = pregion->next;
            }
            if (pregion == NULL || pregion->priority != prio) {
                continue;
            }

            if (r->units) {
                for (u = r->units; u; u = u->next) {
                    processor *porder, *punit = pregion;

                    if (IS_PAUSED(u->faction)) continue;

                    while (punit && punit->priority == prio && punit->type == PR_UNIT) {
                        punit->data.per_unit.process(u);
                        punit = punit->next;
                    }
                    if (punit == NULL || punit->priority != prio) {
                        continue;
                    }

                    porder = punit;
                    while (porder && porder->priority == prio && porder->type == PR_ORDER) {
                        order **ordp = &u->orders;
                        if (porder->flags & PROC_THISORDER) {
                            ordp = &u->thisorder;
                        }
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
                                    else if (!long_order_allowed(u, false)) {
                                        ord = NULL;
                                    }
                                }
                                if (ord) {
                                    porder->data.per_order.process(u, ord);
                                    if (!u->orders) {
                                        /* GIVE UNIT or QUIT delete all orders of the unit, stop */
                                        break;
                                    }
                                }
                            }
                            if (!ord || *ordp == ord) {
                                ordp = &(*ordp)->next;
                            }
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
            if (pregion == NULL || pregion->priority != prio) {
                continue;
            }
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
    const race *rc = u_race(u);
    if (!(rc->flags & RCF_NOWEAPONS)) {
        if (effskill(u, SK_WEAPONLESS, NULL) >= 1) {
            /* kann ohne waffen bewachen: fuer drachen */
            n = u->number;
        }
        else {
            /* alle Waffen werden gezaehlt, und dann wird auf die Anzahl
            * Personen minimiert */
            for (itm = u->items; itm; itm = itm->next) {
                if (rc_can_use(rc, itm->type)) {
                    const weapon_type *wtype = resource2weapon(itm->type->rtype);
                    if (wtype == NULL || (!siege_weapons && (wtype->flags & WTF_SIEGE)))
                        continue;
                    if (effskill(u, wtype->skill, NULL) >= 1)
                        n += itm->number;
                    if (n >= u->number)
                        break;
                }
            }
            if (n > u->number) n = u->number;
        }
    }
    return n;
}

static void enter_1(region * r)
{
    do_enter(r, false);
}

static void enter_2(region * r)
{
    do_enter(r, true);
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
    add_proc_global(p, update_long_orders, "Lange Befehle aktualisieren");
    add_proc_order(p, K_BANNER, banner_cmd, 0, NULL);
    add_proc_order(p, K_EMAIL, email_cmd, 0, NULL);
    add_proc_order(p, K_PASSWORD, password_cmd, 0, NULL);
    add_proc_order(p, K_SEND, send_cmd, 0, NULL);
    add_proc_order(p, K_GROUP, group_cmd, 0, NULL);

    p += 10;
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
    add_proc_order(p, K_LEAVE, leave_cmd, 0, "Verlassen");

    p += 10;
    add_proc_region(p, enter_1, "Betreten (2. Versuch)"); /* to allow a buildingowner to enter the castle pre combat */

    p += 10;
    add_proc_global(p, do_battles, "Attackieren");

    p += 10;                      /* can't allow reserve before siege (weapons) */
    add_proc_region(p, enter_1, "Betreten (3. Versuch)");  /* to claim a castle after a victory and to be able to DESTROY it in the same turn */
    if (config_get_int("rules.reserve.twophase", 0)) {
        add_proc_order(p, K_RESERVE, reserve_self, 0, "RESERVE (self)");
        p += 10;
    }
    add_proc_order(p, K_RESERVE, reserve_cmd, 0, "RESERVE (all)");
    add_proc_order(p, K_CLAIM, claim_cmd, 0, NULL);

    p += 10;                      /* in case it has any effects on alliance victories */
    add_proc_order(p, K_GIVE, give_control_cmd, 0, "GIB KOMMANDO");
    add_proc_order(p, K_FORGET, forget_cmd, 0, "Vergessen");

    p += 10;                      /* reset rng again before economics */
    if (rule_force_leave(FORCE_LEAVE_ALL)) {
        add_proc_region(p, do_force_leave, "kick non-allies out of buildings/ships");
    }
    add_proc_region(p, do_give, "Geben");
    add_proc_region(p+1, recruit, "Rekrutieren");
    add_proc_region(p+2, destroy, "Zerstoeren");
    add_proc_unit(p, follow_cmds, "Folge auf Einheiten setzen");
    add_proc_order(p, K_QUIT, quit_cmd, 0, "Stirb");

    /* all recruitment must be finished before we can calculate 
     * promotion cost of ability */
    p += 10;
    add_proc_global(p, quit, "Sterben");
    add_proc_order(p, K_PROMOTION, promotion_cmd, 0, "Heldenbefoerderung");

    p += 10;
    if (!keyword_disabled(K_PAY)) {
        add_proc_order(p, K_PAY, pay_cmd, 0, "Gebaeudeunterhalt (BEZAHLE NICHT)");
    }
    add_proc_postregion(p, maintain_buildings, "Gebaeudeunterhalt");

    if (!keyword_disabled(K_CAST)) {
        p += 10;
        add_proc_global(p, magic, "Zaubern");
    }

    p += 10;
    add_proc_region(p, do_autostudy, "study automation");
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
    p += 10;

    if (!keyword_disabled(K_SORT)) {
        add_proc_region(p, do_sort, "Einheiten sortieren");
    }
    add_proc_order(p, K_EXPEL, expel_cmd, 0, "Einheiten verjagen");
    if (!keyword_disabled(K_NUMBER)) {
        add_proc_order(p, K_NUMBER, renumber_cmd, 0, "Neue Nummern (Einheiten)");
        p += 10;
        add_proc_global(p, renumber_factions, "Neue Nummern");
    }
    add_proc_order(p, K_LOCALE, locale_cmd, 0, "Sprache wechseln");
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
                owner = update_region_owners(r);
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
    if (config_get_int("modules.wormhole", 1)) {
        wormholes_update();
    }

    /* immer ausfuehren, wenn neue Sprueche dazugekommen sind, oder sich
     * Beschreibungen geaendert haben */
    update_spells();

    /* am Ende der Auswertung die neuen Defaults zu den Befehlen dazu */
    update_defaults();

    /* start the next week */
    ++turn;
}

typedef enum cansee_t {
    CANSEE_DETECTED,
    CANSEE_HIDDEN,
    CANSEE_INVISIBLE
} cansee_t;

static enum cansee_t cansee_ex(const unit *u, const region *r, const unit *target, int stealth, int rings)
{
    enum cansee_t result = CANSEE_HIDDEN;
    if (rings >= target->number) {
        const resource_type *rtype = get_resourcetype(R_AMULET_OF_TRUE_SEEING);
        if (rtype) {
            int amulet = i_get(u->items, rtype->itype);
            if (amulet <= 0) {
                return CANSEE_INVISIBLE;
            }
        }
        else {
            return CANSEE_INVISIBLE;
        }
    }
    if (skill_enabled(SK_PERCEPTION)) {
        int watch = effskill(u, SK_PERCEPTION, r);
        if (stealth > watch) {
            return result;
        }
    }
    return CANSEE_DETECTED;
}

static bool is_exposed(const unit *u) {
    return u->building || u->ship || is_guard(u) || leftship(u);
}

static bool big_sea_monster(const unit *u, const region *r) {
    return ((r->terrain->flags & SEA_REGION) && (u_race(u)->weight >= 5000));
}

bool
cansee_unit(const unit * u, const region *r, const unit * target, int modifier)
/* r kann != u->region sein, wenn es um durchreisen geht */
{
    int stealth, rings;
    enum cansee_t see;
    if (target->number == 0) {
        return false;
    }
    else if (target->faction == u->faction || omniscient(u->faction)) {
        return true;
    }
    else if (is_exposed(target)) {
        return true;
    }
    else if (target->number == 0) {
        attrib *a = a_find(target->attribs, &at_creator);
        if (a) {
            /* u is an empty temporary unit. In this special case we look at the creating unit. */
            target = (unit *)a->data.v;
        }
        else {
            return false;
        }
    }

    stealth = eff_stealth(target, r) - modifier;
    rings = invisible(target, NULL);
    see = cansee_ex(u, r, target, stealth, rings);
    if (CANSEE_HIDDEN == see) {
        /* bug 2763 and 2754: can see sea serpents on oceans */
        return big_sea_monster(u, r);
    }
    return CANSEE_DETECTED == see;
}

/**
 * Determine if unit can be seen by faction.
 *
 * @param f -- the observing faction
 * @param u -- the unit that is observed
 * @param r -- the region that u is obesrved from (see below)
 * @param m -- terrain modifier to stealth
 *
 * r kann != u->region sein, wenn es um Durchreisen geht,
 * oder Zauber (sp_generous, sp_fetchastral).
 * Es muss auch niemand aus f in der region sein, wenn sie vom Turm
 * erblickt wird.
 */
bool cansee(const faction *f, const region *r, const unit *u, int modifier)
{
    unit *u2;
    int rings, stealth;
    bool bsm, result;

    /* quick exits: */
    if (u->faction == f || omniscient(f)) {
        return true;
    }
    else if (u->number == 0) {
        /* no need to do this in each cansee_unit: */
        attrib *a = a_find(u->attribs, &at_creator);
        if (a) {
            /* u is an empty temporary unit. In this special case we look at the creating unit. */
            u = (unit *)a->data.v;
        }
        else {
            return true;
        }
    }
    if (is_exposed(u)) {
        /* obviously visible, we only need a viewer in the region */
        return true;
    }

    rings = invisible(u, NULL);
    stealth = eff_stealth(u, r) - modifier;
    if (stealth < 0 && rings < u->number) return true;
    result = bsm = big_sea_monster(u, r);
    for (u2 = r->units; u2; u2 = u2->next) {
        if (u2->faction == f) {
            enum cansee_t see = cansee_ex(u2, r, u, stealth, rings);
            if (see == CANSEE_DETECTED) {
                return true;
            }
            else if (see == CANSEE_HIDDEN && bsm) {
                return true;
            }
            /* still invisible to all: */
            result = false;
        }
    }
    return result;
}

bool
seefaction(const faction * f, const region * r, const unit * u, int modifier)
{
    if (((f == u->faction) || !fval(u, UFL_ANON_FACTION))
        && cansee(f, r, u, modifier)) {
        return true;
    }
    return false;
}

int locale_cmd(unit * u, order * ord)
{
    char token[128];
    const char * name;
    faction *f = u->faction;
    bool del = config_get_int("translate.delete_orders", 0) != 0;

    init_order(ord, f->locale);
    name = gettoken(token, sizeof(token));
    if (name) {
        const struct locale *lang = get_locale(name);
        if (lang && lang != f->locale) {
            change_locale(f, lang, del);
        }
    }
    return 0;
}

static void expel_building(unit *u, unit *u2, order *ord) {
    building *b = u->building;

    if (u != building_owner(b)) {
        /* error: must be the owner */
        cmistake(u, ord, 5, MSG_EVENT);
    }
    else {
        if (leave(u2, true)) {
            message *msg = msg_message("force_leave_building", "owner unit building", u, u2, u->building);
            add_message(&u->faction->msgs, msg);
            add_message(&u2->faction->msgs, msg);
            msg_release(msg);
        }
    }
}

static void expel_ship(unit *u, unit *u2, order *ord) {
    ship *sh = u->ship;
    if (u != ship_owner(sh)) {
        /* error: must be the owner */
        cmistake(u, ord, 146, MSG_EVENT);
    }
    else if (!u->region->land) {
        /* error: must not be at sea */
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "error_onlandonly", NULL));
    }
    else {
        if (leave(u2, true)) {
            message *msg = msg_message("force_leave_ship", "owner unit ship", u, u2, u->ship);
            add_message(&u->faction->msgs, msg);
            add_message(&u2->faction->msgs, msg);
            msg_release(msg);
        }
    }
}

int expel_cmd(unit *u, order *ord) {
    faction *f = u->faction;
    unit *u2;
    init_order(ord, f->locale);
    getunit(u->region, u->faction, &u2);
    if (u2 == NULL || u2->region != u->region) {
        /* error: target unit not found */
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "feedback_unit_not_found", NULL));
        return 0;
    }
    if (u->building) {
        expel_building(u, u2, ord);
    }
    else if (u->ship) {
        expel_ship(u, u2, ord);
    }
    else {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "feedback_not_inside", NULL));
        /* error: unit must be owner of a ship or building */
    }
    return 0;
}
