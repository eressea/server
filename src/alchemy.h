#ifndef H_KRNL_ALCHEMY_H
#define H_KRNL_ALCHEMY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct potion_type;
    struct unit;
    struct region;
    struct faction;
    struct item_type;
    struct order;

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

    void new_potiontype(struct item_type * itype, int level);
    int potion_level(const struct item_type *itype);
    void show_potions(struct faction *f, int sklevel);

    void herbsearch(struct unit *u, int max);
    int use_potion(struct unit *u, const struct item_type *itype,
        int amount, struct order *);

    int get_effect(const struct unit *u, const struct item_type *effect);
    int change_effect(struct unit *u, const struct item_type *effect,
        int value);
    bool display_potions(struct unit *u);

    /* TODO: rausnehmen, sobald man attribute splitten kann: */
    typedef struct effect_data {
        const struct item_type *type;
        int value;
    } effect_data;

#ifdef __cplusplus
}
#endif
#endif                          /* ALCHEMY_H */
