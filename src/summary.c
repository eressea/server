#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <kernel/config.h>

#include "summary.h"
#include "laws.h"
#include "monsters.h"
#include "kernel/calendar.h"

#include <kernel/alliance.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/skills.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/path.h>
#include <util/unicode.h>

#include <stb_ds.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    long long int playermoney;
    long long int peasantmoney;
    int armed_men;
    int poprace[MAXRACES];
    int factionrace[MAXRACES];
    int landregionen;
    int regionen_mit_spielern;
    int landregionen_mit_spielern;
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

int update_nmrs(int since)
{
    int newplayers = 0;
    faction *f;
    int timeout = NMRTimeout();

    if (timeout>0) {
        if (nmrs == NULL) {
            nmrs = calloc((1 + (size_t)timeout), sizeof(int));
            if (!nmrs) abort();
        }
        else {
            memset(nmrs, 0, sizeof(int) * timeout);
        }
    }
    
    for (f = factions; f; f = f->next) {
        if (faction_age(f)<=1) {
            ++newplayers;
        }
        else if (!fval(f, FFL_NOIDLEOUT | FFL_CURSED)) {
            int nmr = since - f->lastorders;
            if (timeout>0) {
                if (nmr < 0 || nmr > timeout) {
                    log_error("faction %s has %d NMR", itoa36(f->no), nmr);
                    if (nmr < 0) nmr = 0;
                    if (nmr > timeout) nmr = timeout;
                }
                if (nmr > 0) {
                    log_debug("faction %s has %d NMR", itoa36(f->no), nmr);
                }
                ++nmrs[nmr];
            }
        }
    }
    return newplayers;
}

static void out_faction(FILE * file, const struct faction *f)
{
    if (alliances != NULL) {
        fprintf(file, "%s (%s/%d) (%.3s/%.3s), %d Einh., %d Pers., %d NMR\n",
            f->name, itoa36(f->no), f_get_alliance(f) ? f->alliance->id : 0,
            LOC(default_locale, rc_name_s(f->race, NAME_SINGULAR)), magic_school[f->magiegebiet],
            f->num_units, f->num_people, turn - f->lastorders - 1);
    }
    else {
        fprintf(file, "%s (%.3s/%.3s), %d Einh., %d Pers., %d NMR\n",
            factionname(f), LOC(default_locale, rc_name_s(f->race, NAME_SINGULAR)),
            magic_school[f->magiegebiet], f->num_units, f->num_people,
            turn - f->lastorders - 1);
    }
}

static char *gamedate2(const struct locale *lang)
{
    static char buf[256];
    gamedate gd;
    const char *week = "a_week", *month = "a_month";

    get_gamedate(turn, &gd);
    if (weeknames2) {
        week = weeknames2[gd.week];
    }
    month = calendar_month(gd.month);
    sprintf(buf, "in %s des Monats %s im Jahre %d %s.",
        LOC(lang, mkname("calendar", week)),
        LOC(lang, mkname("calendar", month)),
        gd.year,
        LOC(lang, mkname("calendar", calendar_era())));
    return buf;
}

static void writeturn(void)
{
    char zText[4096];
    FILE *f;

    path_join(basepath(), "datum", zText, sizeof(zText));
    f = fopen(zText, "w");
    if (!f) {
        perror(zText);
        return;
    }
    fputs(gamedate2(default_locale), f);
    fclose(f);
    path_join(basepath(), "turn", zText, sizeof(zText));
    f = fopen(zText, "w");
    if (!f) {
        perror(zText);
        return;
    }
    fprintf(f, "%d\n", turn);
    fclose(f);
}

static int count_umlaut(const char *s)
{
    int result = 0;
    const char *cp;
    for (cp = s; *cp; ++cp) {
        wchar_t wc = *cp;
        if (wc & 0x80) {
            size_t size;
            int err;
            err = utf8_decode(&wc, cp, &size);
            if (err != 0) {
                log_error("illegal utf8 encoding %s at %s", s, cp);
                return result;
            }
            cp += size;
            ++result;
        }
    }
    return result;
}

static void summarize_races(const summary *s, FILE *F, bool full) {
    int i;
    for (i = 0; i < MAXRACES; i++) {
        if (s->poprace[i] > 0) {
            const char *pad = "      ";
            int lpad = (int)strlen(pad);
            const race *rc = get_race(i);
            const char *rcname = LOC(default_locale, rc_name_s(rc, NAME_PLURAL));
            lpad -= count_umlaut(rcname);
            assert(lpad >= 0);
            if (full) {
                fputs(pad + lpad, F);
                fprintf(F, "%20s: ", rcname);
                fprintf(F, "%8d\n", s->poprace[i]);
            }
            else if (i != RC_TEMPLATE && i != RC_CLONE) {
                if (playerrace(rc)) {
                    fputs(pad + lpad, F);
                    fprintf(F, "%16s: ", rcname);
                    fprintf(F, "%8d\n", s->poprace[i]);
                }
            }
        }
    }
}

static void summarize_players(const summary *s, FILE *F) {
    int i;
    const char * suffix = LOC(default_locale, "stat_tribe_p");

    for (i = 0; i < MAXRACES; i++) {
        if (i != RC_TEMPLATE && i != RC_CLONE && s->factionrace[i]) {
            const race *rc = get_race(i);
            if (rc && playerrace(rc)) {
                const char * pad = "      ";
                int lpad = (int)strlen(pad);
                const char *rccat = LOC(default_locale, rc_name_s(rc, NAME_CATEGORY));
                lpad -= count_umlaut(rccat);
                assert(lpad >= 0);
                fputs(pad + lpad, F);
                fprintf(F, "%16s%s:", rccat, suffix);
                fprintf(F, "%8d\n", s->factionrace[i]);
            }
        }
    }
}

typedef struct nmr_faction {
    faction *f;
    int nmr;
} nmr_faction;

static int cmp_nmr_faction(const void *a, const void *b)
{
    const nmr_faction *lhs = (const nmr_faction *)a;
    const nmr_faction *rhs = (const nmr_faction *)b;
    if (lhs->nmr == rhs->nmr) {
        return (lhs->f->uid > rhs->f->uid) ? 1 : -1;
    };
    return (lhs->nmr < rhs->nmr) ? 1 : -1;
}

static void report_nmrs(FILE *F, int timeout)
{
    nmr_faction *nmrs = NULL;
    size_t len, i;
    faction *f;
    for (f = factions; f; f = f->next) {
        if (turn - 1 - f->lastorders > 0) {
            struct nmr_faction *nmr = arraddnptr(nmrs, 1);
            nmr->f = f;
            nmr->nmr = turn - 1 - f->lastorders;
        }
    }
    len = arrlen(nmrs);
    qsort(nmrs, len, sizeof(struct nmr_faction), cmp_nmr_faction);
    fprintf(F, "\n\nFactions with NMRs:\n");
    for (i = 0; i != len; ++i) {
        out_faction(F, nmrs[i].f);
    }
    arrfree(nmrs);
}

void report_summary(const summary * s, bool full)
{
    FILE *F = NULL;
    int newplayers = 0;
    faction *f;
    char zText[4096];
    int timeout = NMRTimeout();

    if (full) {
        path_join(basepath(), "parteien.full", zText, sizeof(zText));
    }
    else {
        path_join(basepath(), "parteien", zText, sizeof(zText));
    }
    F = fopen(zText, "w");
    if (!F) {
        perror(zText);
        return;
    }
    log_info("writing summary to file: parteien.\n");
    fprintf(F, "%s\n%s\n\n", game_name(), gamedate2(default_locale));
    fprintf(F, "Auswertung Nr:         %8d\n\n", turn);
    fprintf(F, "Parteien:              %8d\n", s->factions);
    fprintf(F, "Einheiten:             %8d\n", s->nunits);
    fprintf(F, "Spielerpopulation:     %8d\n", s->playerpop);
    fprintf(F, " davon bewaffnet:      %8d\n", s->armed_men);
    fprintf(F, " Helden:               %8d\n", s->heroes);

    if (full) {
        fprintf(F, "Regionen:              %8d\n", (int)listlen(regions));
        fprintf(F, "Bewohnte Regionen:     %8d\n", s->inhabitedregions);
        fprintf(F, "Landregionen:          %8d\n", s->landregionen);
        fprintf(F, "Spielerregionen:       %8d\n", s->regionen_mit_spielern);
        fprintf(F, "Landspielerregionen:   %8d\n", s->landregionen_mit_spielern);
        fprintf(F, "Inaktive Vulkane:      %8d\n", s->inactive_volcanos);
        fprintf(F, "Aktive Vulkane:        %8d\n\n", s->active_volcanos);
    }

    summarize_players(s, F);

    if (full) {
        fprintf(F, "\n");
        {
            struct language *plang = s->languages;
            while (plang != NULL) {
                fprintf(F, "Sprache %2s:            %8d\n", locale_name(plang->locale),
                    plang->number);
                plang = plang->next;
            }
        }
    }

    fprintf(F, "\n");
    summarize_races(s, F, full);

    if (full) {
        fprintf(F, "\nWaffen:               %8d\n", s->waffen);
        fprintf(F, "Ruestungen:           %8d\n", s->ruestungen);
        fprintf(F, "ungezaehmte Pferde:   %8d\n", s->pferde);
        fprintf(F, "gezaehmte Pferde:     %8d\n", s->spielerpferde);
        fprintf(F, "Schiffe:              %8d\n", s->schiffe);
        fprintf(F, "Gebaeude:             %8d\n", s->gebaeude);

        fprintf(F, "\nBauernpopulation:     %8d\n", s->peasants);

        fprintf(F, "Population gesamt:    %8d\n\n", s->playerpop + s->peasants);

        fprintf(F, "Reichtum Spieler: %12lld Silber\n", s->playermoney);
        fprintf(F, "Reichtum Bauern:  %12lld Silber\n", s->peasantmoney);
        fprintf(F, "Reichtum gesamt:  %12lld Silber\n\n", 
            s->playermoney + s->peasantmoney);
    }

    fprintf(F, "\n");

    newplayers = update_nmrs(turn - 1);

    if (nmrs) {
        int i;
        for (i = 0; i <= timeout; ++i) {
            if (i == timeout) {
                fprintf(F, "+ NMR: %3d\n", nmrs[i]);
            }
            else {
                fprintf(F, "%d NMR: %3d\n", i, nmrs[i]);
            }
        }
    }
    if (newbies[2] != 0) {
        fprintf(F, "Erstabgaben:  %3d%%\n", 100 - (dropouts[0] * 100 / newbies[2]));
    }
    if (newbies[3] != 0) {
        fprintf(F, "Zweitabgaben: %3d%%\n", 100 - (dropouts[1] * 100 / newbies[3]));
    }
    fprintf(F, "Neue Spieler: %d\n", newplayers);

    if (full) {
        if (factions) {
            fprintf(F, "\nParteien:\n\n");
            for (f = factions; f; f = f->next) {
                out_faction(F, f);
            }
        }
        if (timeout > 0 && full) {
            report_nmrs(F, timeout);
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
    while (sum->languages) {
        struct language *next = sum->languages->next;
        free(sum->languages);
        sum->languages = next;
    }
    free(sum);
}

summary *make_summary(void)
{
    faction *f;
    region *r;
    unit *u;
    summary *sum = calloc(1, sizeof(summary));
    const struct resource_type *rhorse = get_resourcetype(R_HORSE);

    for (f = factions; f; f = f->next) {
        const struct locale *lang = f->locale;
        struct language *plang = sum->languages;
        while (plang && plang->locale != lang)
            plang = plang->next;
        if (!plang) {
            plang = calloc(1, sizeof(struct language));
            plang->next = sum->languages;
            sum->languages = plang;
            plang->locale = lang;
        }
        ++plang->number;
        if (f->units) {
            sum->factions++;
            /* Problem mit Monsterpartei ... */
            if (!IS_MONSTERS(f)) {
                int rc = old_race(f->race);
                if (rc >= 0 && rc < MAXRACES) {
                    sum->factionrace[rc]++;
                }
            }
        }
    }

    /* count everything */

    for (r = regions; r; r = r->next) {
        sum->pferde += rhorses(r);
        sum->schiffe += listlen(r->ships);
        sum->gebaeude += listlen(r->buildings);
        if (!fval(r->terrain, SEA_REGION)) {
            sum->landregionen++;
            if (r->units) {
                sum->landregionen_mit_spielern++;
            }
            if (r->terrain == newterrain(T_VOLCANO)) {
                sum->inactive_volcanos++;
            }
            else if (r->terrain == newterrain(T_VOLCANO_SMOKING)) {
                sum->active_volcanos++;
            }
        }
        if (r->units) {
            sum->regionen_mit_spielern++;
        }
        if (rpeasants(r) || r->units) {
            sum->inhabitedregions++;
            sum->peasants += rpeasants(r);
            sum->peasantmoney += rmoney(r);

            for (u = r->units; u; u = u->next) {
                race_t orace;
                f = u->faction;
                if (!IS_MONSTERS(u->faction)) {
                    size_t s, len;
                    item *itm;

                    sum->nunits++;
                    sum->playerpop += u->number;
                    if (u->flags & UFL_HERO) {
                        sum->heroes += u->number;
                    }
                    sum->spielerpferde += i_get(u->items, rhorse->itype);
                    sum->playermoney += get_money(u);
                    sum->armed_men += armedmen(u, true);
                    for (itm = u->items; itm; itm = itm->next) {
                        if (itm->type->rtype->wtype) {
                            sum->waffen += itm->number;
                        }
                        if (itm->type->rtype->atype) {
                            sum->ruestungen += itm->number;
                        }
                    }

                    sum->spielerpferde += i_get(u->items, rhorse->itype);

                    for (len = arrlen(u->skills), s = 0; s != len; ++s) {
                        skill_t sk = u->skills[s].id;
                        int aktskill = effskill(u, sk, r);
                        if (aktskill > sum->maxskill) {
                            sum->maxskill = aktskill;
                        }
                    }
                }

                orace = old_race(u_race(u));
                if (orace >= 0 && orace < MAXRACES) {
                    sum->poprace[orace] += u->number;
                }
            }
        }
    }

    return sum;
}
