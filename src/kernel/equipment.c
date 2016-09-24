/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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
#include "faction.h"
#include "race.h"
#include "spell.h"

/* util includes */
#include <quicklist.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static equipment *equipment_sets;

equipment *create_equipment(const char *eqname)
{
    equipment **eqp = &equipment_sets;
    for (;;) {
        struct equipment *eq = *eqp;
        int i = eq ? strcmp(eq->name, eqname) : 1;
        if (i > 0) {
            eq = (equipment *)calloc(1, sizeof(equipment));
            eq->name = _strdup(eqname);
            eq->next = *eqp;
            memset(eq->skills, 0, sizeof(eq->skills));
            *eqp = eq;
            break;
        }
        else if (i == 0) {
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
            eq->skills[sk] = _strdup(value);
        }
        else if (eq->skills[sk]) {
            free(eq->skills[sk]);
        }
    }
}

typedef struct lazy_spell {
    char *name;
    struct spell *sp;
    int level;
} lazy_spell;

void equipment_addspell(equipment * eq, const char * name, int level)
{
    if (eq) {
        lazy_spell *ls = malloc(sizeof(lazy_spell));
        ls->sp = NULL;
        ls->level = level;
        ls->name = _strdup(name);
        ql_push(&eq->spells, ls);
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
                idata = (itemdata *)malloc(sizeof(itemdata));
                idata->itype = itype;
                idata->value = _strdup(value);
                idata->next = eq->items;
                eq->items = idata;
            }
        }
    }
}

void
equipment_setcallback(struct equipment *eq,
void(*callback) (const struct equipment *, struct unit *))
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
            int sk;
            for (sk = 0; sk != MAXSKILLS; ++sk) {
                if (eq->skills[sk] != NULL) {
                    int i = dice_rand(eq->skills[sk]);
                    if (i > 0) {
                        set_level(u, (skill_t)sk, i);
                        if (sk == SK_STAMINA) {
                            u->hp = unit_max_hp(u) * u->number;
                        }
                    }
                }
            }
        }

        if (mask & EQUIP_SPELLS) {
            if (eq->spells) {
                quicklist * ql = eq->spells;
                int qi;
                sc_mage * mage = get_mage(u);

                for (qi = 0; ql; ql_advance(&ql, &qi, 1)) {
                    lazy_spell *sbe = (lazy_spell *)ql_get(ql, qi);
                    if (!sbe->sp) {
                        sbe->sp = find_spell(sbe->name);
                        free(sbe->name);
                        sbe->name = NULL;
                    }
                    unit_add_spell(u, mage, sbe->sp, sbe->level);
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
                    double rnd = (1 + rng_int() % 1000) / 1000.0;
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
                    double rnd = (1 + rng_int() % 1000) / 1000.0;
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

void free_ls(void *arg) {
    lazy_spell *ls = (lazy_spell*)arg;
    free(ls->name);
    free(ls);
}

void equipment_done(void) {
    equipment **eqp = &equipment_sets;
    while (*eqp) {
        int i;
        equipment *eq = *eqp;
        *eqp = eq->next;
        free(eq->name);
        if (eq->spells) {
            ql_foreach(eq->spells, free_ls);
            ql_free(eq->spells);
        }
        while (eq->items) {
            itemdata *next = eq->items->next;
            free(eq->items->value);
            free(eq->items);
            eq->items = next;
        }
        // TODO: subsets, skills
        for (i=0;i!=MAXSKILLS;++i) {
            free(eq->skills[i]);
        }
        free(eq);
    }
}
