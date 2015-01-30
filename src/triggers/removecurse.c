/*
Copyright (c) 1998-2015, Enno Rehling Rehling <enno@eressea.de>
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
#include "removecurse.h"

/* kernel includes */
#include <kernel/curse.h>
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

#include <stdio.h>

typedef struct removecurse_data {
    curse *curse;
    unit *target;
} removecurse_data;

static void removecurse_init(trigger * t)
{
    t->data.v = calloc(sizeof(removecurse_data), 1);
}

static void removecurse_free(trigger * t)
{
    free(t->data.v);
}

static int removecurse_handle(trigger * t, void *data)
{
    /* call an event handler on removecurse.
     * data.v -> ( variant event, int timer )
     */
    removecurse_data *td = (removecurse_data *)t->data.v;
    if (td->curse && td->target) {
        if (!remove_curse(&td->target->attribs, td->curse)) {
            log_error("could not perform removecurse::handle()\n");
        }
    }
    unused_arg(data);
    return 0;
}

static void removecurse_write(const trigger * t, struct storage *store)
{
    removecurse_data *td = (removecurse_data *)t->data.v;
    WRITE_TOK(store, td->target ? itoa36(td->target->no) : 0);
    WRITE_INT(store, td->curse ? td->curse->no : 0);
}

static int removecurse_read(trigger * t, struct storage *store)
{
    removecurse_data *td = (removecurse_data *)t->data.v;

    read_reference(&td->target, store, read_unit_reference, resolve_unit);
    read_reference(&td->curse, store, read_int, resolve_curse);

    return AT_READ_OK;
}

trigger_type tt_removecurse = {
    "removecurse",
    removecurse_init,
    removecurse_free,
    removecurse_handle,
    removecurse_write,
    removecurse_read
};

trigger *trigger_removecurse(curse * c, unit * target)
{
    trigger *t = t_new(&tt_removecurse);
    removecurse_data *td = (removecurse_data *)t->data.v;
    td->curse = c;
    td->target = target;
    return t;
}
