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

#ifndef H_KRNL_ALCHEMY_H
#define H_KRNL_ALCHEMY_H
#ifdef __cplusplus
extern "C" {
#endif

enum {
	/* Stufe 1 */
	P_FAST,
	P_STRONG,
	P_LIFE,
	/* Stufe 2 */
	P_DOMORE,
	P_HEILWASSER,
	P_BAUERNBLUT,
	/* Stufe 3 */
	P_WISE,						/* 6 */
	P_FOOL,
#ifdef INSECT_POTION
	P_WARMTH,
#else
	P_STEEL,
#endif
	P_HORSE,
	P_BERSERK,					/* 10 */
	/* Stufe 4 */
	P_PEOPLE,
	P_WAHRHEIT,
	P_MACHT,
	P_HEAL,
	MAX_POTIONS,
	NOPOTION = (potion_t) - 1
};

enum {
	H_PLAIN_1,
	H_PLAIN_2,
	H_PLAIN_3,
	H_FOREST_1,
	H_FOREST_2,
	H_FOREST_3,
	H_SWAMP_1,
	H_SWAMP_2,
	H_SWAMP_3,
	H_DESERT_1,
	H_DESERT_2,
	H_DESERT_3,
	H_HIGHLAND_1,
	H_HIGHLAND_2,
	H_HIGHLAND_3,
	H_MOUNTAIN_1,
	H_MOUNTAIN_2,
	H_MOUNTAIN_3,
	H_GLACIER_1,
	H_GLACIER_2,
	H_GLACIER_3,
	MAX_HERBS,
	NOHERB = (herb_t) - 1
};

herb_t rherb2herb(struct region *r);
void herbsearch(struct region * r, struct unit * u, int max);
int use_potion(struct unit * u, const struct item_type * itype, int amount, struct order *);
void init_potions(void);

extern int get_effect(const struct unit * u, const struct potion_type * effect);
extern int change_effect(struct unit * u, const struct potion_type * effect, int value);
extern attrib_type at_effect;

/* rausnehmen, sobald man attribute splitten kann: */
typedef struct effect_data {
	const struct potion_type * type;
	int value;
} effect_data;

#ifdef __cplusplus
}
#endif
#endif				/* ALCHEMY_H */
