#pragma once

#include <stdbool.h>

struct unit;
struct order;
struct item_type;

void register_itemfunctions(void);

int use_bloodpotion(struct unit* u, const struct item_type* itype,
    int amount, struct order* ord);

