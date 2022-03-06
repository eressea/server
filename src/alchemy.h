#pragma once
#ifndef H_KRNL_ALCHEMY_H
#define H_KRNL_ALCHEMY_H

#include <stdbool.h>

extern struct attrib_type at_effect;

enum {
    /* Stufe 1 */
    P_FAST,
    P_STRONG,
    P_LIFE,
    /* Stufe 2 */
    P_DOMORE,
    P_OINTMENT,
    P_BAUERNBLUT,
    /* Stufe 3 */
    P_WISE,                     /* 6 */
    P_FOOL,
    P_STEEL,
    P_HORSE,
    P_BERSERK,                  /* 10 */
    /* Stufe 4 */
    P_PEOPLE,
    P_WAHRHEIT,
    P_HEAL,
    MAX_POTIONS
};

struct item_type;
struct faction;
struct unit;
struct order;
struct attrib;

void clone_effects(const struct unit* src, struct unit* dst);
void scale_effects(struct attrib* alist, int n, int size);

void new_potiontype(struct item_type* itype, int level);
int potion_level(const struct item_type* itype);
void show_potions(struct faction* f, int sklevel);

void herbsearch(struct unit* u, int max);
int use_potion(struct unit* u, const struct item_type* itype,
    int amount, struct order*);

int get_effect(const struct unit* u, const struct item_type* effect);
int change_effect(struct unit* u, const struct item_type* effect,
    int value);
bool display_potions(struct unit* u);

int effect_value(const struct attrib* a);
const struct item_type *effect_type(const struct attrib* a);

#endif                          /* ALCHEMY_H */
