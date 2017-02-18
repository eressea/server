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
#include "changefaction.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/faction.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/base36.h>

#include <storage.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <stdio.h>
/***
 ** restore a mage that was turned into a toad
 **/

typedef struct changefaction_data {
    struct unit *unit;
    struct faction *faction;
} changefaction_data;

static void changefaction_init(trigger * t)
{
    t->data.v = calloc(sizeof(changefaction_data), 1);
}

static void changefaction_free(trigger * t)
{
    free(t->data.v);
}

static int changefaction_handle(trigger * t, void *data)
{
    /* call an event handler on changefaction.
     * data.v -> ( variant event, int timer )
     */
    changefaction_data *td = (changefaction_data *)t->data.v;
    if (td->unit && td->faction) {
        u_setfaction(td->unit, td->faction);
    }
    else {
        log_error("could not perform changefaction::handle()\n");
    }
    UNUSED_ARG(data);
    return 0;
}

static void changefaction_write(const trigger * t, struct storage *store)
{
    changefaction_data *td = (changefaction_data *)t->data.v;
    write_unit_reference(td->unit, store);
    write_faction_reference(td->faction->_alive ? td->faction : NULL, store);
}

static int changefaction_read(trigger * t, gamedata *data)
{
    variant var;
    changefaction_data *td = (changefaction_data *)t->data.v;

    read_reference(&td->unit, data, read_unit_reference, resolve_unit);
    var = read_faction_reference(data);
    if (var.i == 0) {
        return AT_READ_FAIL;
    }
    ur_add(var, &td->faction, resolve_faction);
    return AT_READ_OK;
}

trigger_type tt_changefaction = {
    "changefaction",
    changefaction_init,
    changefaction_free,
    changefaction_handle,
    changefaction_write,
    changefaction_read
};

trigger *trigger_changefaction(unit * u, struct faction * f)
{
    trigger *t = t_new(&tt_changefaction);
    changefaction_data *td = (changefaction_data *)t->data.v;
    td->unit = u;
    td->faction = f;
    return t;
}
