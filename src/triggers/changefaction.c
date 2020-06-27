#include <platform.h>
#include "changefaction.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/faction.h>

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
    t->data.v = calloc(1, sizeof(changefaction_data));
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
        unit * u = td->unit;
        u_setfaction(u, td->faction);
        u_freeorders(u);
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
    changefaction_data *td = (changefaction_data *)t->data.v;

    read_unit_reference(data, &td->unit, NULL);
    return read_faction_reference(data, &td->faction) > 0 ? AT_READ_OK : AT_READ_FAIL;
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
