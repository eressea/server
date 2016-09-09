/* 
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2007   |
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *
 */

/* wenn platform.h nicht vor curses included wird, kompiliert es unter windows nicht */
#include <platform.h>
#include <kernel/config.h>

#include "summary.h"
#include "laws.h"
#include "monster.h"
#include "calendar.h"

#include <kernel/alliance.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#undef SUMMARY_BOM              /* write a BOM in the summary file */

typedef struct summary {
    int waffen;
    int factions;
    int ruestungen;
    int schiffe;
    int gebaeude;
    int maxskill;
    int heroes;
    int inhabitedregions;
    int peasants;
    int nunits;
    int playerpop;
    double playermoney;
    double peasantmoney;
    int armed_men;
    int poprace[MAXRACES];
    int factionrace[MAXRACES];
    int landregionen;
    int regionen_mit_spielern;
    int landregionen_mit_spielern;
    int orkifizierte_regionen;
    int inactive_volcanos;
    int active_volcanos;
    int spielerpferde;
    int pferde;
    struct language {
        struct language *next;
        int number;
        const struct locale *locale;
    } *languages;
} summary;


int *nmrs = NULL;

int update_nmrs(void)
{
    int i, newplayers = 0;
    faction *f;
    int turn = global.data_turn;

    if (nmrs == NULL)
        nmrs = malloc(sizeof(int) * (NMRTimeout() + 1));
    for (i = 0; i <= NMRTimeout(); ++i) {
        nmrs[i] = 0;
    }

    for (f = factions; f; f = f->next) {
        if (fval(f, FFL_ISNEW)) {
            ++newplayers;
        }
        else if (!fval(f, FFL_NOIDLEOUT|FFL_CURSED)) {
            int nmr = turn - f->lastorders + 1;
            if (nmr < 0 || nmr > NMRTimeout()) {
                log_error("faction %s has %d NMR", factionid(f), nmr);
                nmr = _max(0, nmr);
                nmr = _min(nmr, NMRTimeout());
            }
            if (nmr > 0) {
                log_debug("faction %s has %d NMR", factionid(f), nmr);
            }
            ++nmrs[nmr];
        }
    }
    return newplayers;
}

static char *pcomp(double i, double j)
{
    static char buf[32];
    sprintf(buf, "%.0f (%s%.0f)", i, (i >= j) ? "+" : "", i - j);
    return buf;
}

static char *rcomp(int i, int j)
{
    static char buf[32];
    sprintf(buf, "%d (%s%d,%s%d%%)",
        i, (i >= j) ? "+" : "", i - j, (i >= j) ? "+" : "",
        j ? ((i - j) * 100) / j : 0);
    return buf;
}

static void out_faction(FILE * file, const struct faction *f)
{
    if (alliances != NULL) {
        fprintf(file, "%s (%s/%d) (%.3s/%.3s), %d Einh., %d Pers., $%d, %d NMR\n",
            f->name, itoa36(f->no), f_get_alliance(f) ? f->alliance->id : 0,
            LOC(default_locale, rc_name_s(f->race, NAME_SINGULAR)), magic_school[f->magiegebiet],
            count_units(f), f->num_total, f->money, turn - f->lastorders);
    }
    else {
        fprintf(file, "%s (%.3s/%.3s), %d Einh., %d Pers., $%d, %d NMR\n",
            factionname(f), LOC(default_locale, rc_name_s(f->race, NAME_SINGULAR)),
            magic_school[f->magiegebiet], count_units(f), f->num_total, f->money,
            turn - f->lastorders);
    }
}

static char *gamedate2(const struct locale *lang)
{
    static char buf[256];
    gamedate gd;
    const char *week = "a_week", *month = "a_month";

    if (weeknames2) {
        week = weeknames2[gd.week];
    }
    if (monthnames) {
        month = monthnames[gd.month];
    }
    get_gamedate(turn, &gd);
    sprintf(buf, "in %s des Monats %s im Jahre %d %s.",
        LOC(lang, week),
        LOC(lang, month),
        gd.year, agename ? LOC(lang, agename) : "");
    return buf;
}

static void writeturn(void)
{
    char zText[MAX_PATH];
    FILE *f;

    join_path(basepath(), "datum", zText, sizeof(zText));
    f = fopen(zText, "w");
    if (!f) {
        perror(zText);
        return;
    }
    fputs(gamedate2(default_locale), f);
    fclose(f);
    join_path(basepath(), "turn", zText, sizeof(zText));
    f = fopen(zText, "w");
    if (!f) {
        perror(zText);
        return;
    }
    fprintf(f, "%d\n", turn);
    fclose(f);
}

void report_summary(summary * s, summary * o, bool full)
{
    FILE *F = NULL;
    int i, newplayers = 0;
    faction *f;
    char zText[MAX_PATH];

    if (full) {
        join_path(basepath(), "parteien.full", zText, sizeof(zText));
    }
    else {
        join_path(basepath(), "parteien", zText, sizeof(zText));
    }
    F = fopen(zText, "w");
    if (!F) {
        perror(zText);
        return;
    }
#ifdef SUMMARY_BOM
    else {
        const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
        fwrite(utf8_bom, 1, 3, F);
    }
#endif
    log_info("writing summary to file: parteien.\n");
    fprintf(F, "%s\n%s\n\n", game_name(), gamedate2(default_locale));
    fprintf(F, "Auswertung Nr:         %d\n\n", turn);
    fprintf(F, "Parteien:              %s\n", pcomp(s->factions, o->factions));
    fprintf(F, "Einheiten:             %s\n", pcomp(s->nunits, o->nunits));
    fprintf(F, "Spielerpopulation:     %s\n", pcomp(s->playerpop, o->playerpop));
    fprintf(F, " davon bewaffnet:      %s\n", pcomp(s->armed_men, o->armed_men));
    fprintf(F, " Helden:               %s\n", pcomp(s->heroes, o->heroes));

    if (full) {
        fprintf(F, "Regionen:              %d\n", listlen(regions));
        fprintf(F, "Bewohnte Regionen:     %d\n", s->inhabitedregions);
        fprintf(F, "Landregionen:          %d\n", s->landregionen);
        fprintf(F, "Spielerregionen:       %d\n", s->regionen_mit_spielern);
        fprintf(F, "Landspielerregionen:   %d\n", s->landregionen_mit_spielern);
        fprintf(F, "Orkifizierte Regionen: %d\n", s->orkifizierte_regionen);
        fprintf(F, "Inaktive Vulkane:      %d\n", s->inactive_volcanos);
        fprintf(F, "Aktive Vulkane:        %d\n\n", s->active_volcanos);
    }

    for (i = 0; i < MAXRACES; i++) {
        if (i != RC_TEMPLATE && i != RC_CLONE && s->factionrace[i]) {
            const race *rc = get_race(i);
            if (rc && playerrace(rc)) {
                fprintf(F, "%13s%s: %s\n", LOC(default_locale, rc_name_s(rc, NAME_CATEGORY)),
                    LOC(default_locale, "stat_tribe_p"), pcomp(s->factionrace[i],
                    o->factionrace[i]));
            }
        }
    }

    if (full) {
        fprintf(F, "\n");
        {
            struct language *plang = s->languages;
            while (plang != NULL) {
                struct language *olang = o->languages;
                int nold = 0;
                while (olang && olang->locale != plang->locale)
                    olang = olang->next;
                if (olang)
                    nold = olang->number;
                fprintf(F, "Sprache %12s: %s\n", locale_name(plang->locale),
                    rcomp(plang->number, nold));
                plang = plang->next;
            }
        }
    }

    fprintf(F, "\n");
    if (full) {
        for (i = 0; i < MAXRACES; i++) {
            if (s->poprace[i]) {
                const race *rc = get_race(i);
                fprintf(F, "%20s: %s\n", LOC(default_locale, rc_name_s(rc, NAME_PLURAL)),
                    rcomp(s->poprace[i], o->poprace[i]));
            }
        }
    }
    else {
        for (i = 0; i < MAXRACES; i++) {
            if (i != RC_TEMPLATE && i != RC_CLONE && s->poprace[i]) {
                const race *rc = get_race(i);
                if (playerrace(rc)) {
                    fprintf(F, "%20s: %s\n", LOC(default_locale, rc_name_s(rc, NAME_PLURAL)),
                        rcomp(s->poprace[i], o->poprace[i]));
                }
            }
        }
    }

    if (full) {
        fprintf(F, "\nWaffen:               %s\n", pcomp(s->waffen, o->waffen));
        fprintf(F, "Ruestungen:           %s\n",
            pcomp(s->ruestungen, o->ruestungen));
        fprintf(F, "ungezaehmte Pferde:   %s\n", pcomp(s->pferde, o->pferde));
        fprintf(F, "gezaehmte Pferde:     %s\n",
            pcomp(s->spielerpferde, o->spielerpferde));
        fprintf(F, "Schiffe:              %s\n", pcomp(s->schiffe, o->schiffe));
        fprintf(F, "Gebaeude:             %s\n", pcomp(s->gebaeude, o->gebaeude));

        fprintf(F, "\nBauernpopulation:     %s\n", pcomp(s->peasants, o->peasants));

        fprintf(F, "Population gesamt:    %d\n\n", s->playerpop + s->peasants);

        fprintf(F, "Reichtum Spieler:     %s Silber\n",
            pcomp(s->playermoney, o->playermoney));
        fprintf(F, "Reichtum Bauern:      %s Silber\n",
            pcomp(s->peasantmoney, o->peasantmoney));
        fprintf(F, "Reichtum gesamt:      %s Silber\n\n",
            pcomp(s->playermoney + s->peasantmoney,
            o->playermoney + o->peasantmoney));
    }

    fprintf(F, "\n\n");

    newplayers = update_nmrs();

    for (i = 0; i <= NMRTimeout(); ++i) {
        if (i == NMRTimeout()) {
            fprintf(F, "+ NMR:\t\t %d\n", nmrs[i]);
        }
        else {
            fprintf(F, "%d NMR:\t\t %d\n", i, nmrs[i]);
        }
    }
    if (age) {
        if (age[2] != 0) {
            fprintf(F, "Erstabgaben:\t %d%%\n", 100 - (dropouts[0] * 100 / age[2]));
        }
        if (age[3] != 0) {
            fprintf(F, "Zweitabgaben:\t %d%%\n", 100 - (dropouts[1] * 100 / age[3]));
        }
    }
    fprintf(F, "Neue Spieler:\t %d\n", newplayers);

    if (full) {
        if (factions)
            fprintf(F, "\nParteien:\n\n");

        for (f = factions; f; f = f->next) {
            out_faction(F, f);
        }

        if (NMRTimeout() && full) {
            fprintf(F, "\n\nFactions with NMRs:\n");
            for (i = NMRTimeout(); i > 0; --i) {
                for (f = factions; f; f = f->next) {
                    if (i == NMRTimeout()) {
                        if (turn - f->lastorders >= i) {
                            out_faction(F, f);
                        }
                    }
                    else {
                        if (turn - f->lastorders == i) {
                            out_faction(F, f);
                        }
                    }
                }
            }
        }
    }

    fclose(F);

    if (full) {
        log_info("writing date & turn\n");
        writeturn();
    }
    free(nmrs);
    nmrs = NULL;
}

void free_summary(summary *sum) {
    free(sum);
}

summary *make_summary(void)
{
    faction *f;
    region *r;
    unit *u;
    summary *s = calloc(1, sizeof(summary));
    const struct resource_type *rhorse = get_resourcetype(R_HORSE);

    for (f = factions; f; f = f->next) {
        const struct locale *lang = f->locale;
        struct language *plang = s->languages;
        while (plang && plang->locale != lang)
            plang = plang->next;
        if (!plang) {
            plang = calloc(sizeof(struct language), 1);
            plang->next = s->languages;
            s->languages = plang;
            plang->locale = lang;
        }
        ++plang->number;
        f->nregions = 0;
        f->num_total = 0;
        f->money = 0;
        if (f->units) {
            s->factions++;
            /* Problem mit Monsterpartei ... */
            if (!is_monsters(f)) {
                int rc = old_race(f->race);
                if (rc >= 0) {
                    s->factionrace[rc]++;
                }
            }
        }
    }

    /* count everything */

    for (r = regions; r; r = r->next) {
        s->pferde += rhorses(r);
        s->schiffe += listlen(r->ships);
        s->gebaeude += listlen(r->buildings);
        if (!fval(r->terrain, SEA_REGION)) {
            s->landregionen++;
            if (r->units) {
                s->landregionen_mit_spielern++;
            }
            if (fval(r, RF_ORCIFIED)) {
                s->orkifizierte_regionen++;
            }
            if (r->terrain == newterrain(T_VOLCANO)) {
                s->inactive_volcanos++;
            }
            else if (r->terrain == newterrain(T_VOLCANO_SMOKING)) {
                s->active_volcanos++;
            }
        }
        if (r->units) {
            s->regionen_mit_spielern++;
        }
        if (rpeasants(r) || r->units) {
            s->inhabitedregions++;
            s->peasants += rpeasants(r);
            s->peasantmoney += rmoney(r);

            /* Einheiten Info. nregions darf nur einmal pro Partei
             * incrementiert werden. */

            for (u = r->units; u; u = u->next)
                freset(u->faction, FFL_SELECT);
            for (u = r->units; u; u = u->next) {
                int orace;
                f = u->faction;
                if (!is_monsters(u->faction)) {
                    skill *sv;
                    item *itm;

                    s->nunits++;
                    s->playerpop += u->number;
                    if (u->flags & UFL_HERO) {
                        s->heroes += u->number;
                    }
                    s->spielerpferde += i_get(u->items, rhorse->itype);
                    s->playermoney += get_money(u);
                    s->armed_men += armedmen(u, true);
                    for (itm = u->items; itm; itm = itm->next) {
                        if (itm->type->rtype->wtype) {
                            s->waffen += itm->number;
                        }
                        if (itm->type->rtype->atype) {
                            s->ruestungen += itm->number;
                        }
                    }

                    s->spielerpferde += i_get(u->items, rhorse->itype);

                    for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
                        skill_t sk = sv->id;
                        int aktskill = effskill(u, sk, r);
                        if (aktskill > s->maxskill)
                            s->maxskill = aktskill;
                    }
                    if (!fval(f, FFL_SELECT)) {
                        f->nregions++;
                        fset(f, FFL_SELECT);
                    }
                }

                f->num_total += u->number;
                f->money += get_money(u);
                orace = (int)old_race(u_race(u));
                if (orace >= 0) {
                    s->poprace[orace] += u->number;
                }
            }
        }
    }

    return s;
}
