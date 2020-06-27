#ifdef _MSC_VER
#include <platform.h>
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
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/path.h>
#include <util/unicode.h>

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

int update_nmrs(void)
{
    int newplayers = 0;
    faction *f;
    int timeout = NMRTimeout();

    if (timeout>0) {
        int i;
        if (nmrs == NULL) {
            nmrs = malloc(sizeof(int) * (timeout + 1));
            if (!nmrs) abort();
        }
        for (i = 0; i <= timeout; ++i) {
            nmrs[i] = 0;
        }
    }
    
    for (f = factions; f; f = f->next) {
        if (f->age<=1) {
            ++newplayers;
        }
        else if (!fval(f, FFL_NOIDLEOUT|FFL_CURSED)) {
            int nmr = turn - f->lastorders;
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
            f->num_units, f->num_people, turn - f->lastorders);
    }
    else {
        fprintf(file, "%s (%.3s/%.3s), %d Einh., %d Pers., %d NMR\n",
            factionname(f), LOC(default_locale, rc_name_s(f->race, NAME_SINGULAR)),
            magic_school[f->magiegebiet], f->num_units, f->num_people,
            turn - f->lastorders);
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
        wint_t wc = *cp;
        if (wc & 0x80) {
            size_t size;
            int err;
            err = unicode_utf8_decode(&wc, cp, &size);
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
#ifdef SUMMARY_BOM
    else {
        const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
        fwrite(utf8_bom, 1, 3, F);
    }
#endif
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

    newplayers = update_nmrs();

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
    if (age) {
        if (age[2] != 0) {
            fprintf(F, "Erstabgaben:  %3d%%\n", 100 - (dropouts[0] * 100 / age[2]));
        }
        if (age[3] != 0) {
            fprintf(F, "Zweitabgaben: %3d%%\n", 100 - (dropouts[1] * 100 / age[3]));
        }
    }
    fprintf(F, "Neue Spieler: %d\n", newplayers);

    if (full) {
        if (factions) {
            fprintf(F, "\nParteien:\n\n");
            for (f = factions; f; f = f->next) {
                out_faction(F, f);
            }
        }

        if (timeout>0 && full) {
            int i;
            fprintf(F, "\n\nFactions with NMRs:\n");
            for (i = timeout; i > 0; --i) {
                for (f = factions; f; f = f->next) {
                    if (i == timeout) {
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
    summary *s = calloc(1, sizeof(summary));
    const struct resource_type *rhorse = get_resourcetype(R_HORSE);

    for (f = factions; f; f = f->next) {
        const struct locale *lang = f->locale;
        struct language *plang = s->languages;
        while (plang && plang->locale != lang)
            plang = plang->next;
        if (!plang) {
            plang = calloc(1, sizeof(struct language));
            plang->next = s->languages;
            s->languages = plang;
            plang->locale = lang;
        }
        ++plang->number;
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
                }

                orace = (int)old_race(u_race(u));
                if (orace >= 0) {
                    s->poprace[orace] += u->number;
                }
            }
        }
    }

    return s;
}
