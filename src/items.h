#ifndef H_KRNL_ITEMS
#define H_KRNL_ITEMS

#include <stdbool.h>

struct unit;

#ifdef __cplusplus
extern "C" {
#endif

    void register_itemfunctions(void);

    int use_bloodpotion(struct unit* u, const struct item_type* itype,
        int amount, struct order* ord);

#ifdef __cplusplus
}
#endif
#endif
