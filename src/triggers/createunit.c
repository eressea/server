#include <platform.h>
#include "createunit.h"

/* kernel includes */
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/region.h>

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

typedef struct createunit_data {
    struct region *r;
    struct faction *f;
    const struct race *race;
    int number;
} createunit_data;

static void createunit_init(trigger * t)
{
    t->data.v = calloc(1, sizeof(createunit_data));
}

static void createunit_free(trigger * t)
{
    free(t->data.v);
}

static int createunit_handle(trigger * t, void *data)
{
    /* call an event handler on createunit.
     * data.v -> ( variant event, int timer )
     */
    createunit_data *td = (createunit_data *)t->data.v;
    if (td->r != NULL && td->f != NULL) {
        create_unit(td->r, td->f, td->number, td->race, 0, NULL, NULL);
    }
    else {
        log_error("could not perform createunit::handle()\n");
    }
    UNUSED_ARG(data);
    return 0;
}

static void createunit_write(const trigger * t, struct storage *store)
{
    createunit_data *td = (createunit_data *)t->data.v;
    write_faction_reference(td->f->_alive ? td->f : NULL, store);
    write_region_reference(td->r, store);
    write_race_reference(td->race, store);
    WRITE_INT(store, td->number);
}

static int createunit_read(trigger * t, gamedata *data)
{
    createunit_data *td = (createunit_data *)t->data.v;
    int id;
    int result = AT_READ_OK;

    id = read_faction_reference(data, &td->f);
    if (id <= 0) {
        result = AT_READ_FAIL;
    }

    read_region_reference(data, &td->r);
    td->race = read_race_reference(data->store);
    if (!td->race) {
        result = AT_READ_FAIL;
    }
    READ_INT(data->store, &td->number);
    return result;
}

trigger_type tt_createunit = {
    "createunit",
    createunit_init,
    createunit_free,
    createunit_handle,
    createunit_write,
    createunit_read
};

trigger *trigger_createunit(region * r, struct faction * f,
    const struct race * rc, int number)
{
    trigger *t = t_new(&tt_createunit);
    createunit_data *td = (createunit_data *)t->data.v;
    td->r = r;
    td->f = f;
    td->race = rc;
    td->number = number;
    return t;
}
