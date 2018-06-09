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

