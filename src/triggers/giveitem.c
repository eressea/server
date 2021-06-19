#include <platform.h>
#include "giveitem.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/unit.h>

/* util includes */
#include <kernel/attrib.h>
#include <util/base36.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <util/macros.h>
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
    t->data.v = calloc(1, sizeof(giveitem_data));
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
    UNUSED_ARG(data);
    return 0;
}

static void giveitem_write(const trigger * t, struct storage *store)
{
    giveitem_data *td = (giveitem_data *)t->data.v;
    write_unit_reference(td->u, store);
    WRITE_INT(store, td->number);
    WRITE_TOK(store, td->itype->rtype->_name);
}

static int giveitem_read(trigger * t, gamedata *data)
{
    giveitem_data *td = (giveitem_data *)t->data.v;
    char zText[128];
    int result;

    result = read_unit_reference(data, &td->u, NULL);

    READ_INT(data->store, &td->number);
    READ_TOK(data->store, zText, sizeof(zText));
    td->itype = it_find(zText);
    assert(td->itype);

    if (result == 0) {
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
    if (t) {
        giveitem_data *td = (giveitem_data *)t->data.v;
        td->number = number;
        td->u = u;
        td->itype = itype;
    }
    return t;
}
