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
#include "equipment.h"

/* kernel includes */
#include "item.h"
#include "unit.h"
#include "race.h"

/* util includes */
#include <util/quicklist.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <string.h>

static equipment *equipment_sets;

equipment *create_equipment(const char *eqname)
{
  equipment **eqp = &equipment_sets;

  for (;;) {
    struct equipment *eq = *eqp;

    int i = eq ? strcmp(eq->name, eqname) : 1;

    if (i > 0) {
      eq = malloc(sizeof(equipment));
      eq->name = strdup(eqname);
      eq->next = *eqp;
      eq->items = NULL;
      eq->spells = NULL;
      eq->subsets = NULL;
      eq->callback = NULL;
      memset(eq->skills, 0, sizeof(eq->skills));
      *eqp = eq;
      break;
    } else if (i == 0) {
      break;
    }
    eqp = &eq->next;
  }
  return *eqp;
}

equipment *get_equipment(const char *eqname)
{
  equipment *eq = equipment_sets;

  for (; eq; eq = eq->next) {
    int i = strcmp(eq->name, eqname);

    if (i == 0)
      return eq;
    else if (i > 0)
      break;
  }
  return NULL;
}

void equipment_setskill(equipment * eq, skill_t sk, const char *value)
{
  if (eq != NULL) {
    if (value != NULL) {
      eq->skills[sk] = strdup(value);
    } else if (eq->skills[sk]) {
      free(eq->skills[sk]);
    }
  }
}

void equipment_addspell(equipment * eq, spell * sp)
{
  if (eq != NULL) {
    ql_set_insert(&eq->spells, sp);
  }
}

void
equipment_setitem(equipment * eq, const item_type * itype, const char *value)
{
  if (eq != NULL) {
    if (itype != NULL) {
      itemdata *idata = eq->items;

      while (idata && idata->itype != itype) {
        idata = idata->next;
      }
      if (idata == NULL) {
        idata = (itemdata *) malloc(sizeof(itemdata));
        idata->itype = itype;
        idata->value = strdup(value);
        idata->next = eq->items;
        eq->items = idata;
      }
    }
  }
}

void
equipment_setcallback(struct equipment *eq,
  void (*callback) (const struct equipment *, struct unit *))
{
  eq->callback = callback;
}

void equip_unit(struct unit *u, const struct equipment *eq)
{
  equip_unit_mask(u, eq, EQUIP_ALL);
}

void equip_unit_mask(struct unit *u, const struct equipment *eq, int mask)
{
  if (eq) {

    if (mask & EQUIP_SKILLS) {
      skill_t sk;

      for (sk = 0; sk != MAXSKILLS; ++sk) {
        if (eq->skills[sk] != NULL) {
          int i = dice_rand(eq->skills[sk]);

          if (i > 0)
            set_level(u, sk, i);
        }
      }
    }

    if (mask & EQUIP_SPELLS) {
      quicklist *ql = eq->spells;

      if (ql) {
        sc_mage *m = get_mage(u);

        if (m == NULL) {
          assert(!"trying to equip spells on a non-mage!");
        } else {
          int qi;

          for (qi = 0; ql; ql_advance(&ql, &qi, 1)) {
            spell *sp = (spell *) ql_get(ql, qi);

            add_spell(get_spelllist(m, u->faction), sp);
          }
        }
      }
    }

    if (mask & EQUIP_ITEMS) {
      itemdata *idata;

      for (idata = eq->items; idata != NULL; idata = idata->next) {
        int i = u->number * dice_rand(idata->value);

        if (i > 0) {
          i_add(&u->items, i_new(idata->itype, i));
        }
      }
    }

    if (eq->subsets) {
      int i;

      for (i = 0; eq->subsets[i].sets; ++i) {
        if (chance(eq->subsets[i].chance)) {
          float rnd = (1 + rng_int() % 1000) / 1000.0f;

          int k;

          for (k = 0; eq->subsets[i].sets[k].set; ++k) {
            if (rnd <= eq->subsets[i].sets[k].chance) {
              equip_unit_mask(u, eq->subsets[i].sets[k].set, mask);
              break;
            }
            rnd -= eq->subsets[i].sets[k].chance;
          }
        }
      }
    }

    if (mask & EQUIP_SPECIAL) {
      if (eq->callback)
        eq->callback(eq, u);
    }
  }
}

void equip_items(struct item **items, const struct equipment *eq)
{
  if (eq) {
    itemdata *idata;

    for (idata = eq->items; idata != NULL; idata = idata->next) {
      int i = dice_rand(idata->value);

      if (i > 0) {
        i_add(items, i_new(idata->itype, i));
      }
    }
    if (eq->subsets) {
      int i;

      for (i = 0; eq->subsets[i].sets; ++i) {
        if (chance(eq->subsets[i].chance)) {
          float rnd = (1 + rng_int() % 1000) / 1000.0f;

          int k;

          for (k = 0; eq->subsets[i].sets[k].set; ++k) {
            if (rnd <= eq->subsets[i].sets[k].chance) {
              equip_items(items, eq->subsets[i].sets[k].set);
              break;
            }
            rnd -= eq->subsets[i].sets[k].chance;
          }
        }
      }
    }
  }
}
