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
#include "artrewards.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/pool.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/save.h>
#include <kernel/curse.h>
#include <kernel/messages.h>
#include <kernel/ship.h>

/* util includes */
#include <util/attrib.h>
#include <util/rand.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#define HORNRANGE 10
#define HORNDURATION 3
#define HORNIMMUNITY 30

static int age_peaceimmune(attrib * a)
{
    return (--a->data.i > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

static attrib_type at_peaceimmune = {
    "peaceimmune",
    NULL, NULL,
    age_peaceimmune,
    a_writeint,
    a_readint
};

static int
use_hornofdancing(struct unit *u, const struct item_type *itype,
int amount, struct order *ord)
{
    region *r;
    int regionsPacified = 0;

    for (r = regions; r; r = r->next) {
        if (distance(u->region, r) < HORNRANGE) {
            if (a_find(r->attribs, &at_peaceimmune) == NULL) {
                attrib *a;

                create_curse(u, &r->attribs, ct_find("peacezone"),
                    20, HORNDURATION, 1.0, 0);

                a = a_add(&r->attribs, a_new(&at_peaceimmune));
                a->data.i = HORNIMMUNITY;

                ADDMSG(&r->msgs, msg_message("hornofpeace_r_success",
                    "unit region", u, u->region));

                regionsPacified++;
            }
            else {
                ADDMSG(&r->msgs, msg_message("hornofpeace_r_nosuccess",
                    "unit region", u, u->region));
            }
        }
    }

    if (regionsPacified > 0) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "hornofpeace_u_success",
            "pacified", regionsPacified));
    }
    else {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "hornofpeace_u_nosuccess",
            ""));
    }

    return 0;
}

#define SPEEDUP 2

static int
useonother_trappedairelemental(struct unit *u, int shipId,
const struct item_type *itype, int amount, struct order *ord)
{
    curse *c;
    ship *sh;

    if (shipId <= 0) {
        cmistake(u, ord, 20, MSG_MOVE);
        return -1;
    }

    sh = findshipr(u->region, shipId);
    if (!sh) {
        cmistake(u, ord, 20, MSG_MOVE);
        return -1;
    }

    c =
        create_curse(u, &sh->attribs, ct_find("shipspeedup"), 20, INT_MAX, SPEEDUP,
        0);
    c_setflag(c, CURSE_NOAGE);

    ADDMSG(&u->faction->msgs, msg_message("trappedairelemental_success",
        "unit region command ship", u, u->region, ord, sh));

    use_pooled(u, itype->rtype, GET_DEFAULT, 1);

    return 0;
}

static int
use_trappedairelemental(struct unit *u,
const struct item_type *itype, int amount, struct order *ord)
{
    ship *sh = u->ship;

    if (sh == NULL) {
        cmistake(u, ord, 20, MSG_MOVE);
        return -1;
    }
    return useonother_trappedairelemental(u, sh->no, itype, amount, ord);
}

void register_artrewards(void)
{
    at_register(&at_peaceimmune);
    register_item_use(use_hornofdancing, "use_hornofdancing");
    register_item_use(use_trappedairelemental, "use_trappedairelemental");
    register_item_useonother(useonother_trappedairelemental,
        "useonother_trappedairelemental");
}
