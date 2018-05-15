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

#ifndef H_KRNL_EQUIPMENT_H
#define H_KRNL_EQUIPMENT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
    struct unit;

#define EQUIP_SKILLS  (1<<1)
#define EQUIP_SPELLS  (1<<2)
#define EQUIP_ITEMS   (1<<3)
#define EQUIP_SPECIAL (1<<4)
#define EQUIP_ALL     (0xFF)
    bool equip_unit_mask(struct unit *u, const char *eqname, int mask);
    bool equip_unit(struct unit *u, const char *eqname);

#ifdef __cplusplus
}
#endif
#endif
