#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <kernel/config.h>
#include "score.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/building.h>
#include <kernel/build.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skills.h>
#include <kernel/unit.h>
#include <kernel/pool.h>

/* util includes */
#include <util/base36.h>
#include <util/language.h>
#include <util/path.h>

#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdio.h>

score_t average_score_of_age(int age)
{
    int range = age / 24 + 1;
    faction *f;
    score_t sum = 0, count = 0;

    for (f = factions; f; f = f->next) {
        if (!fval(f, FFL_NPC)) {
            int f_age = faction_age(f);
            if (f_age <= age + range && f_age >= age - range && f->race != get_race(RC_TEMPLATE)) {
                sum += f->score;
                count++;
            }
        }
    }

    if (count == 0) {
        return 0;
    }
    return sum / count;
}

void update_scores(void)
{
    region *r;
    faction *fc;
    score_t allscores = 0;

    for (fc = factions; fc; fc = fc->next)
        fc->score = 0;

    for (r = regions; r; r = r->next) {
        unit *u;
        building *b;
        ship *s;

        if (rule_region_owners()) {
            fc = region_get_owner(r);
            if (fc)
                fc->score += 10;
        }

        for (b = r->buildings; b; b = b->next) {
            u = building_owner(b);
            if (u != NULL) {
                faction *fbo = u->faction;

                if (fbo) {
                    fbo->score += b->size * 5;
                }
            }
        }

        for (s = r->ships; s; s = s->next) {
            unit *cap = ship_owner(s);
            if (cap && cap->faction) {
                cap->faction->score += s->size * 2;
            }
        }

        for (u = r->units; u; u = u->next) {
            item *itm;
            int itemscore = 0;
            size_t s, len;
            faction *f = u->faction;
            const race *rc = u_race(u);

            if (f == NULL) {
                continue;
            }
            else if (rc->recruitcost > 0) {
                f->score += (rc->recruitcost * u->number) / 50;
            }
            f->score += get_money(u) / 50;
            for (itm = u->items; itm; itm = itm->next) {
                itemscore += itm->number * itm->type->score;
            }
            f->score += itemscore / 10;

            for (len = arrlen(u->skills), s = 0; s != len; ++s) {
                skill *sv = u->skills + s;
                switch (sv->id) {
                case SK_MAGIC:
                    f->score += (score_t)(u->number * pow(sv->level, 4));
                    break;
                case SK_TACTICS:
                    f->score += (score_t)(u->number * pow(sv->level, 3));
                    break;
                case SK_SPY:
                case SK_ALCHEMY:
                case SK_HERBALISM:
                    f->score += (score_t)(u->number * pow(sv->level, 2.5));
                    break;
                default:
                    f->score += (score_t)(u->number * pow(sv->level, 2.5) / 10);
                    break;
                }
            }
        }
    }

    for (fc = factions; fc; fc = fc->next) {
        fc->score = fc->score / 5;
        if (!fval(fc, FFL_NPC) && fc->race != get_race(RC_TEMPLATE)) {
            allscores += fc->score;
        }
    }
    if (allscores == 0) {
        allscores = 1;
    }
}

void write_scores(void)
{
    FILE *scoreFP;
    char path[4096];

    path_join(basepath(), "score", path, sizeof(path));
    scoreFP = fopen(path, "w");
    if (scoreFP) {
        faction *f;
        for (f = factions; f; f = f->next)
            if (!fval(f, FFL_NPC) && f->num_people != 0) {
                char score[32];
                int age = faction_age(f);
                write_score(score, sizeof(score), f->score);
                fprintf(scoreFP, "%s ", score);
                write_score(score, sizeof(score), average_score_of_age(age));
                fprintf(scoreFP, "(%s) ", score);
                fprintf(scoreFP, "%30.30s (%3.3s) %5s (%3d)\n",
                    f->name,
                    f->race->_name,
                    itoa36(f->no),
                    age);
            }
        fclose(scoreFP);
    }
    if (alliances != NULL) {
        alliance *a;
        const item_type *token = it_find("conquesttoken");

        path_join(basepath(), "score.alliances", path, sizeof(path));
        scoreFP = fopen(path, "w");
        if (scoreFP) {
            fprintf(scoreFP, "# alliance:factions:persons:score\n");

            for (a = alliances; a; a = a->next) {
                score_t alliance_score = 0;
                int alliance_number = 0, alliance_factions = 0, grails = 0;
                faction *f;

                for (f = factions; f; f = f->next) {
                    if (f->alliance && f->alliance == a) {
                        alliance_factions++;
                        alliance_score += f->score;
                        alliance_number += f->num_people;
                        if (token != NULL) {
                            unit *u = f->units;
                            while (u != NULL) {
                                item **iitem = i_find(&u->items, token);
                                if (iitem != NULL && *iitem != NULL) {
                                    grails += (*iitem)->number;
                                }
                                u = u->nextF;
                            }
                        }
                    }
                }

                fprintf(scoreFP, "%d:%d:%d:%lld", a->id, alliance_factions,
                    alliance_number, alliance_score);
                if (token != NULL)
                    fprintf(scoreFP, ":%d", grails);
                fputc('\n', scoreFP);
            }
            fclose(scoreFP);
        }
    }
}

void score(void)
{
    update_scores();
    write_scores();
}

int default_score(const item_type *itype) {
    int result = 0;
    if (itype->rtype->wtype || itype->rtype->atype) {
        result += 10;
    }
    if (itype->construction) {
        requirement *req = itype->construction->materials;
        while (req->number) {
            int score = req->rtype->itype ? req->rtype->itype->score : 10;
            result += score * req->number * 2;
            ++req;
        }
    }
    else {
        result = 10;
    }
    return result;
}

void write_score(char *buffer, size_t size, score_t score) {
    snprintf(buffer, size, "%lld", score);
}
