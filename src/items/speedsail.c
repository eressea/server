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
#include "speedsail.h"

/* kernel includes */
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <attributes/movement.h>

/* util includes */
#include <util/attrib.h>
#include <util/log.h>

/* libc includes */
#include <assert.h>

static int
use_speedsail(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    struct plane *p = rplane(u->region);
    unused_arg(amount);
    unused_arg(itype);
    if (p != NULL) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "use_realworld_only", ""));
    }
    else {
        if (u->ship) {
            attrib *a = a_find(u->ship->attribs, &at_speedup);
            if (a == NULL) {
                a = a_add(&u->ship->attribs, a_new(&at_speedup));
                a->data.sa[0] = 50;     /* speed */
                a->data.sa[1] = 50;     /* decay */
                ADDMSG(&u->faction->msgs, msg_message("use_speedsail", "unit", u));
                /* Ticket abziehen */
                i_change(&u->items, itype, -1);
                return 0;
            }
            else {
                cmistake(u, ord, 211, MSG_EVENT);
            }
        }
        else {
            cmistake(u, ord, 144, MSG_EVENT);
        }
    }
    return EUNUSABLE;
}

void register_speedsail(void)
{
    register_item_use(use_speedsail, "use_speedsail");
}
