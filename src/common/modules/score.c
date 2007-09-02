/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#ifdef SCORE_MODULE
#include "score.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/unit.h>
#include <kernel/pool.h>

/* util includes */
#include <base36.h>
#include <goodies.h>

/* libc includes */
#include <math.h>

int
average_score_of_age(int age, int a)
{
  faction *f;
  int sum = 0, count = 0;

  for (f = factions; f; f = f->next) {
    if (f->no != MONSTER_FACTION && f->race != new_race[RC_TEMPLATE] && f->age <= age + a && f->age >= age - a) {
      sum += f->score;
      count++;
    }
  }

  if (count == 0) {
    return 0;
  }
  return sum / count;
}

void
score(void)
{
  FILE *scoreFP;
  region *r;
  faction *f;
  int allscores = 0;
  char path[MAX_PATH];

  for (f = factions; f; f = f->next) f->score = 0;

  for (r = regions; r; r = r->next) {
    unit * u;
    building * b;
    ship * s;

    for (b = r->buildings; b; b = b->next) {
      u = buildingowner(r, b);
      if (u!=NULL) {
        unit * u2;
        faction * fbo = u->faction;
        int c = b->size - u->number;

        if (fbo) {
          if (u->number <= b->size)
            fbo->score += u->number * 5;

          fbo->score += b->size;
        }

        for (u2 = r->units; u2 && c > 0; u2 = u2->next) {
          if (u2->building == b && u2 != u && u2->number <= c) {
            c -= u2->number;

            if (u2->faction) {
              if (u2->faction == fbo) {
                u2->faction->score += u2->number * 5;
              } else {
                u2->faction->score += u2->number * 3;
              }
            }
          }
        }
      }
    }
    for (s = r->ships; s; s = s->next) {
      unit * u = shipowner(s);
      if (u && u->faction) {
        u->faction->score += s->size * 2;
      }
    }
    for (u = r->units; u; u = u->next) {
      item * itm;
      int itemscore = 0;
      int i;
      faction * f = u->faction;

      if (f==NULL || u->race == new_race[RC_SPELL] || u->race == new_race[RC_BIRTHDAYDRAGON]) {
        continue;
      }

      if (old_race(u->race) <= RC_AQUARIAN) {
        f->score += (u->race->recruitcost * u->number) / 50;
      }
      f->score += get_money(u) / 50;
      for (itm=u->items; itm; itm=itm->next) {
        itemscore += itm->number * itm->type->score;
      }
      f->score += itemscore / 10;

      for (i=0;i!=u->skill_size;++i) {
        skill * sv = u->skills+i;
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

  for (f = factions; f; f = f->next) {
    f->score = f->score / 5;
    if (f->no != MONSTER_FACTION && f->race != new_race[RC_TEMPLATE]) {
      allscores += f->score;
    }
  }
  if (allscores == 0) {
    allscores = 1;
  }

  sprintf(path, "%s/score", basepath());
  scoreFP = fopen(path, "w");
  if (scoreFP) {
    const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf };
    fwrite(utf8_bom, 1, 3, scoreFP);
    for (f = factions; f; f = f->next) if (f->num_total != 0) {
      fprintf(scoreFP, "%8d (%8d/%4.2f%%/%5.2f) %30.30s (%3.3s) %5s (%3d)\n",
        f->score, f->score - average_score_of_age(f->age, f->age / 24 + 1),
        ((float) f->score / (float) allscores) * 100.0,
        (float) f->score / f->num_total,
        f->name, LOC(default_locale, rc_name(f->race, 0)), factionid(f), f->age);
    }
    fclose(scoreFP);
  }
  if (alliances!=NULL) {
    alliance *a;
    const item_type * token = it_find("conquesttoken");

    sprintf(path, "%s/score.alliances", basepath());
    scoreFP = fopen(path, "w");
    if (scoreFP) {
      const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf };
      fwrite(utf8_bom, 1, 3, scoreFP);

      fprintf(scoreFP, "# alliance:factions:persons:score\n");

      for (a = alliances; a; a = a->next) {
        int alliance_score = 0, alliance_number = 0, alliance_factions = 0;
        int grails = 0;

        for (f = factions; f; f = f->next) {
          if (f->alliance && f->alliance->id == a->id) {
            alliance_factions++;
            alliance_score  += f->score;
            alliance_number += f->num_total;
            if (token!=NULL) {
              unit * u = f->units;
              while (u!=NULL) {
                item ** iitem = i_find(&u->items, token);
                if (iitem!=NULL && *iitem!=NULL) {
                  grails += (*iitem)->number;
                }
                u=u->nextF;
              }
            }
          }
        }

        fprintf(scoreFP, "%d:%d:%d:%d", a->id, alliance_factions, alliance_number, alliance_score);
        if (token!=NULL) fprintf(scoreFP, ":%d", grails);
        fputc('\n', scoreFP);
      }
      fclose(scoreFP);
    }
  }
}

#endif
