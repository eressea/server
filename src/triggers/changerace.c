#include <platform.h>
#include "changerace.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/race.h>

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

typedef struct changerace_data {
    struct unit *u;
    const struct race *race;
    const struct race *irace;
} changerace_data;

static void changerace_init(trigger * t)
{
    t->data.v = calloc(1, sizeof(changerace_data));
}

static void changerace_free(trigger * t)
{
    free(t->data.v);
}

static int changerace_handle(trigger * t, void *data)
{
    /* call an event handler on changerace.
     * data.v -> ( variant event, int timer )
     */
    changerace_data *td = (changerace_data *)t->data.v;
    if (td->u) {
        if (td->race != NULL)
            u_setrace(td->u, td->race);
        if (td->irace != NULL)
            td->u->irace = td->irace;
    }
    else {
        log_error("could not perform changerace::handle()\n");
    }
    UNUSED_ARG(data);
    return 0;
}

static void changerace_write(const trigger * t, struct storage *store)
{
    changerace_data *td = (changerace_data *)t->data.v;
    write_unit_reference(td->u, store);
    write_race_reference(td->race, store);
    write_race_reference(td->irace, store);
}

static int changerace_read(trigger * t, gamedata *data)
{
    changerace_data *td = (changerace_data *)t->data.v;
    read_unit_reference(data, &td->u, NULL);
    td->race = read_race_reference(data->store);
    td->irace = read_race_reference(data->store);
    return AT_READ_OK;
}

trigger_type tt_changerace = {
    "changerace",
    changerace_init,
    changerace_free,
    changerace_handle,
    changerace_write,
    changerace_read
};

trigger *trigger_changerace(unit * u, const race * prace, const race * irace)
{
    trigger *t = t_new(&tt_changerace);
    changerace_data *td = (changerace_data *)t->data.v;

    td->u = u;
    td->race = prace;
    td->irace = irace;
    return t;
}
