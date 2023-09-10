#pragma once

#include <stdbool.h>

struct castorder;
struct order;
struct unit;
struct region;
struct fighter;
struct item_type;
struct resource_type;

struct callback_struct {
    bool (*equip_unit)(struct unit *u, const char *eqname, int mask);
    int (*cast_spell)(struct castorder *co, const char *fname);
    int (*use_item)(struct unit *u, const struct item_type *itype,
        int amount, struct order *ord);
    void(*produce_resource)(struct region *, const struct resource_type *, int);
    int(*limit_resource)(const struct region *, const struct resource_type *);
    void (*report_special_attacks)(const struct fighter *fig, const struct item_type *itype);
};

extern struct callback_struct callbacks;
