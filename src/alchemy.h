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

#ifndef H_KRNL_ALCHEMY_H
#define H_KRNL_ALCHEMY_H

#ifdef __cplusplus
extern "C" {
#endif

    struct potion_type;
    struct unit;
    struct region;
    struct item_type;
    struct order;

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
        P_WISE,                     /* 6 */
        P_FOOL,
#ifdef INSECT_POTION
        P_WARMTH,
#else
        P_STEEL,
#endif
        P_HORSE,
        P_BERSERK,                  /* 10 */
        /* Stufe 4 */
        P_PEOPLE,
        P_WAHRHEIT,
        P_MACHT,
        P_HEAL,
        MAX_POTIONS
    };

    void herbsearch(struct region *r, struct unit *u, int max);
    extern int use_potion(struct unit *u, const struct item_type *itype,
        int amount, struct order *);
    extern int use_potion_delayed(struct unit *u, const struct item_type *itype,
        int amount, struct order *);
    extern void init_potions(void);


    extern int get_effect(const struct unit *u, const struct potion_type *effect);
    extern int change_effect(struct unit *u, const struct potion_type *effect,
        int value);
    extern struct attrib_type at_effect;

    /* rausnehmen, sobald man attribute splitten kann: */
    typedef struct effect_data {
        const struct potion_type *type;
        int value;
    } effect_data;

#ifdef __cplusplus
}
#endif
#endif                          /* ALCHEMY_H */
