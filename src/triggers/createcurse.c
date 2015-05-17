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
#include "createcurse.h"

/* kernel includes */
#include <kernel/version.h>
#include <kernel/curse.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
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

typedef struct createcurse_data {
    struct unit *mage;
    struct unit *target;
    const curse_type *type;
    double vigour;
    int duration;
    double effect;
    int men;
} createcurse_data;

static void createcurse_init(trigger * t)
{
    t->data.v = calloc(sizeof(createcurse_data), 1);
}

static void createcurse_free(trigger * t)
{
    free(t->data.v);
}

static int createcurse_handle(trigger * t, void *data)
{
    /* call an event handler on createcurse.
     * data.v -> ( variant event, int timer )
     */
    createcurse_data *td = (createcurse_data *)t->data.v;
    if (td->mage && td->target && td->mage->number && td->target->number) {
        create_curse(td->mage, &td->target->attribs,
            td->type, td->vigour, td->duration, td->effect, td->men);
    }
    else {
        log_error("could not perform createcurse::handle()\n");
    }
    unused_arg(data);
    return 0;
}

static void createcurse_write(const trigger * t, struct storage *store)
{
    createcurse_data *td = (createcurse_data *)t->data.v;
    write_unit_reference(td->mage, store);
    write_unit_reference(td->target, store);
    WRITE_TOK(store, td->type->cname);
    WRITE_FLT(store, (float)td->vigour);
    WRITE_INT(store, td->duration);
    WRITE_FLT(store, (float)td->effect);
    WRITE_INT(store, td->men);
}

static int createcurse_read(trigger * t, struct storage *store)
{
    createcurse_data *td = (createcurse_data *)t->data.v;
    char zText[128];
    float flt;

    read_reference(&td->mage, store, read_unit_reference, resolve_unit);
    read_reference(&td->target, store, read_unit_reference, resolve_unit);

    READ_TOK(store, zText, sizeof(zText));
    td->type = ct_find(zText);
    READ_FLT(store, &flt);
    td->vigour = flt;
    READ_INT(store, &td->duration);
    if (global.data_version < CURSEFLOAT_VERSION) {
        int n;
        READ_INT(store, &n);
        td->effect = (float)n;
    }
    else {
        READ_FLT(store, &flt);
        td->effect = flt;
    }
    READ_INT(store, &td->men);
    return AT_READ_OK;
}

trigger_type tt_createcurse = {
    "createcurse",
    createcurse_init,
    createcurse_free,
    createcurse_handle,
    createcurse_write,
    createcurse_read
};

trigger *trigger_createcurse(struct unit * mage, struct unit * target,
    const curse_type * ct, double vigour, int duration, double effect, int men)
{
    trigger *t = t_new(&tt_createcurse);
    createcurse_data *td = (createcurse_data *)t->data.v;
    td->mage = mage;
    td->target = target;
    td->type = ct;
    td->vigour = vigour;
    td->duration = duration;
    td->effect = effect;
    td->men = men;
    return t;
}
