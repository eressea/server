/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
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
#include "equipment.h"

/* kernel includes */
#include "item.h"
#include "unit.h"
#include "race.h"

/* util includes */
#include <util/rand.h>

/* libc includes */
#include <string.h>

static equipment * equipment_sets;

equipment *
create_equipment(const char * eqname)
{
  equipment ** eqp = &equipment_sets;
  for (;;) {
    struct equipment * eq = *eqp;
    int i = eq?strcmp(eq->name, eqname):1;
    if (i>0) {
      eq = malloc(sizeof(equipment));
      eq->name = strdup(eqname);
      eq->next = *eqp;
      eq->items = NULL;
      eq->spells = NULL;
      eq->subsets = NULL;
      memset(eq->skills, 0, sizeof(eq->skills));
      *eqp = eq;
      break;
    } else if (i==0) {
      break;
    }
    eqp = &eq->next;
  }
  return *eqp;
}

equipment *
get_equipment(const char * eqname)
{
  equipment * eq = equipment_sets;
  for (;eq;eq=eq->next) {
    int i = strcmp(eq->name, eqname);
    if (i==0) return eq;
    else if (i>0) break;
  }
  return NULL;
}

void
equipment_setskill(equipment * eq, skill_t sk, const char * value)
{
  if (eq!=NULL) {
    if (value!=NULL) {
      eq->skills[sk] = strdup(value);
    } else if (eq->skills[sk]) {
      free(eq->skills[sk]);
    }
  }
}

void
equipment_addspell(equipment * eq, spell * sp)
{
  if (eq!=NULL) {
    spelllist_add(&eq->spells, sp);
  }
}

void 
equipment_setitem(equipment * eq, const item_type * itype, const char * value)
{
  if (eq!=NULL) {
    if (itype!=NULL) {
      itemdata * idata = eq->items;
      while (idata &&idata->itype!=itype) {
        idata = idata->next;
      }
      if (idata==NULL) {
        idata = malloc(sizeof(itemdata));
        idata->itype = itype;
        idata->value = strdup(value);
        idata->next = eq->items;
        eq->items = idata;
      }
    }
  }
}

void
equip_unit(struct unit * u, const struct equipment * eq)
{
  if (eq) {
    skill_t sk;
    itemdata * idata;
    sc_mage * m = get_mage(u);
    if (m!=NULL) {
      spell_list * sp = eq->spells;
      while (sp) {
        add_spell(m, sp->data);
        sp = sp->next;
      }
    }
    for (sk=0;sk!=MAXSKILLS;++sk) {
      if (eq->skills[sk]!=NULL) {
        int i = dice_rand(eq->skills[sk]);
        if (i>0) set_level(u, sk, i);
      }
    }
    for (idata=eq->items;idata!=NULL;idata=idata->next) {
      int i = u->number * dice_rand(idata->value);
      if (i>0) {
        i_add(&u->items, i_new(idata->itype, i));
      }
    }
    if (eq->subsets) {
      int i;
      for (i=0;eq->subsets[i].sets;++i) {
        if (chance(eq->subsets[i].chance)) {
          float rnd = 1000.0f / (1+rand() % 1000);
          int k;
          for (k=0;eq->subsets[i].sets[k].set;++k) {
            if (rnd<=eq->subsets[i].sets[k].chance) {
              equip_unit(u, eq->subsets[i].sets[k].set);
              break;
            }
            rnd -= eq->subsets[i].sets[k].chance;
          }
        }
      }
    }
  }
}
