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
#include "giveitem.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>

#include <storage.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/***
 ** give an item to someone
 **/

typedef struct giveitem_data {
    struct unit *u;
    const struct item_type *itype;
    int number;
} giveitem_data;

static void giveitem_init(trigger * t)
{
    t->data.v = calloc(sizeof(giveitem_data), 1);
}

static void giveitem_free(trigger * t)
{
    free(t->data.v);
}

static int giveitem_handle(trigger * t, void *data)
{
    /* call an event handler on giveitem.
     * data.v -> ( variant event, int timer )
     */
    giveitem_data *td = (giveitem_data *)t->data.v;
    if (td->u && td->u->number) {
        i_change(&td->u->items, td->itype, td->number);
    }
    else {
        log_error("could not perform giveitem::handle()\n");
    }
    unused_arg(data);
    return 0;
}

static void giveitem_write(const trigger * t, struct storage *store)
{
    giveitem_data *td = (giveitem_data *)t->data.v;
    write_unit_reference(td->u, store);
    WRITE_INT(store, td->number);
    WRITE_TOK(store, td->itype->rtype->_name);
}

static int giveitem_read(trigger * t, struct storage *store)
{
    giveitem_data *td = (giveitem_data *)t->data.v;
    char zText[128];

    int result = read_reference(&td->u, store, read_unit_reference, resolve_unit);

    READ_INT(store, &td->number);
    READ_TOK(store, zText, sizeof(zText));
    td->itype = it_find(zText);
    assert(td->itype);

    if (result == 0 && td->u == NULL) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

trigger_type tt_giveitem = {
    "giveitem",
    giveitem_init,
    giveitem_free,
    giveitem_handle,
    giveitem_write,
    giveitem_read
};

trigger *trigger_giveitem(unit * u, const item_type * itype, int number)
{
    trigger *t = t_new(&tt_giveitem);
    giveitem_data *td = (giveitem_data *)t->data.v;
    td->number = number;
    td->u = u;
    td->itype = itype;
    return t;
}
