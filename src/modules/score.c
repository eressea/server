/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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
#if SCORE_MODULE
#include "score.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>
#include <kernel/pool.h>

/* util includes */
#include <util/base36.h>
#include <util/language.h>

/* libc includes */
#include <math.h>
#include <stdio.h>

int average_score_of_age(int age, int a)
{
  faction *f;
  int sum = 0, count = 0;

  for (f = factions; f; f = f->next) {
      if (!fval(f, FFL_NPC) && f->age <= age + a
          && f->age >= age - a && f->race != get_race(RC_TEMPLATE)) {
      sum += f->score;
      count++;
    }
  }

  if (count == 0) {
    return 0;
  }
  return sum / count;
}

void score(void)
{
  FILE *scoreFP;
  region *r;
  faction *fc;
  int allscores = 0;
  char path[MAX_PATH];

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
      int i;
      faction *f = u->faction;

      if (f == NULL || u_race(u) == get_race(RC_SPELL)
          || u_race(u) == get_race(RC_BIRTHDAYDRAGON)) {
        continue;
      }

      if (old_race(u_race(u)) <= RC_AQUARIAN) {
        f->score += (u_race(u)->recruitcost * u->number) / 50;
      }
      f->score += get_money(u) / 50;
      for (itm = u->items; itm; itm = itm->next) {
        itemscore += itm->number * itm->type->score;
      }
      f->score += itemscore / 10;

      for (i = 0; i != u->skill_size; ++i) {
        skill *sv = u->skills + i;
        switch (sv->id) {
          case SK_MAGIC:
            f->score += (int)(u->number * pow(sv->level, 4));
            break;
          case SK_TACTICS:
            f->score += (int)(u->number * pow(sv->level, 3));
            break;
          case SK_SPY:
          case SK_ALCHEMY:
          case SK_HERBALISM:
            f->score += (int)(u->number * pow(sv->level, 2.5));
            break;
          default:
            f->score += (int)(u->number * pow(sv->level, 2.5) / 10);
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

  sprintf(path, "%s/score", basepath());
  scoreFP = fopen(path, "w");
  if (scoreFP) {
    const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
    faction *f;
    fwrite(utf8_bom, 1, 3, scoreFP);
    for (f = factions; f; f = f->next)
      if (f->num_total != 0) {
        fprintf(scoreFP, "%8d (%8d/%4.2f%%/%5.2f) %30.30s (%3.3s) %5s (%3d)\n",
          f->score, f->score - average_score_of_age(f->age, f->age / 24 + 1),
          ((float)f->score / (float)allscores) * 100.0,
          (float)f->score / f->num_total,
          f->name, LOC(default_locale, rc_name_s(f->race, NAME_SINGULAR)), factionid(f),
          f->age);
      }
    fclose(scoreFP);
  }
  if (alliances != NULL) {
    alliance *a;
    const item_type *token = it_find("conquesttoken");

    sprintf(path, "%s/score.alliances", basepath());
    scoreFP = fopen(path, "w");
    if (scoreFP) {
      const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf, 0 };
      fwrite(utf8_bom, 1, 3, scoreFP);

      fprintf(scoreFP, "# alliance:factions:persons:score\n");

      for (a = alliances; a; a = a->next) {
        int alliance_score = 0, alliance_number = 0, alliance_factions = 0;
        int grails = 0;
        faction *f;

        for (f = factions; f; f = f->next) {
          if (f->alliance && f->alliance == a) {
            alliance_factions++;
            alliance_score += f->score;
            alliance_number += f->num_total;
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

        fprintf(scoreFP, "%d:%d:%d:%d", a->id, alliance_factions,
          alliance_number, alliance_score);
        if (token != NULL)
          fprintf(scoreFP, ":%d", grails);
        fputc('\n', scoreFP);
      }
      fclose(scoreFP);
    }
  }
}

#endif
