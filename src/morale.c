/*
Copyright (c) 1998-2014,
Enno Rehling <enno@eressea.de>
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

#include <platform.h>
#include "morale.h"

#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/building.h>

#include <util/rand.h>

#include <assert.h>

static double popularity()
{
    return 1.0 / (MORALE_AVERAGE - MORALE_COOLDOWN); /* 10 turns average */
}

void morale_update(region *r) {
    int morale = region_get_morale(r);
    assert(r->land);

    if (r->land->ownership && r->land->ownership->owner) {
        int stability = turn - r->land->ownership->morale_turn;
        int maxmorale = MORALE_DEFAULT;
        building *b = largestbuilding(r, &cmp_taxes, false);
        if (b) {
            int bsize = buildingeffsize(b, false);
            maxmorale = (int)(0.5 + b->type->taxes(b, bsize + 1) / MORALE_TAX_FACTOR);
        }
        if (morale < maxmorale) {
            if (stability > MORALE_COOLDOWN && r->land->ownership->owner
                && morale < MORALE_MAX) {
                double ch = popularity();
                if (is_cursed(r->attribs, C_GENEROUS, 0)) {
                    ch *= 1.2;            /* 20% improvement */
                }
                if (stability >= MORALE_AVERAGE * 2 || chance(ch)) {
                    region_set_morale(r, morale + 1, turn);
                }
            }
        }
        else if (morale > maxmorale) {
            region_set_morale(r, morale - 1, turn);
        }
    }
    else if (morale > MORALE_DEFAULT) {
        region_set_morale(r, morale - 1, turn);
    }
}

void morale_change(region *r, int value) {
    int morale = region_get_morale(r);
    if (morale > 0) {
        morale = MAX(0, morale - value);
        region_set_morale(r, morale, turn);
    }
}
