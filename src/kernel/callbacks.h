#ifndef H_KRNL_CALLBACKS_H
#define H_KRNL_CALLBACKS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct castorder;
    struct order;
    struct unit;
    struct region;
    struct item_type;
    struct resource_type;

    struct callback_struct {
        bool (*equip_unit)(struct unit *u, const char *eqname, int mask);
        int (*cast_spell)(struct castorder *co, const char *fname);
        int (*use_item)(struct unit *u, const struct item_type *itype,
            int amount, struct order *ord);
        void(*produce_resource)(struct region *, const struct resource_type *, int);
        int(*limit_resource)(const struct region *, const struct resource_type *);
    };

    extern struct callback_struct callbacks;
#ifdef __cplusplus
}
#endif
#endif /* H_KRNL_CALLBACKS_H */

