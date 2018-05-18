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

#include <platform.h>
#include <kernel/config.h>
#include "equipment.h"

/* kernel includes */
#include "callbacks.h"
#include "faction.h"
#include "item.h"
#include "race.h"
#include "spell.h"
#include "unit.h"

/* util includes */
#include <selist.h>
#include <critbit.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/strings.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

bool equip_unit(struct unit *u, const char *eqname)
{
    return equip_unit_mask(u, eqname, EQUIP_ALL);
}

bool equip_unit_mask(struct unit *u, const char *eqname, int mask)
{
    if (callbacks.equip_unit) {
        return callbacks.equip_unit(u, eqname, mask);
    }
    return false;
}
