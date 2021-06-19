#include <platform.h>
#include "timeout.h"

/* util includes */
#include <kernel/attrib.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <util/macros.h>

#include <storage.h>

#include <stdio.h>
#include <stdlib.h>
/***
 ** timeout
 **/

static void timeout_init(trigger * t)
{
    t->data.v = calloc(1, sizeof(timeout_data));
}

static void timeout_free(trigger * t)
{
    timeout_data *td = (timeout_data *)t->data.v;
    free_triggers(td->triggers);
    free(t->data.v);
}

static int timeout_handle(trigger * t, void *data)
{
    /* call an event handler on timeout.
     * data.v -> ( variant event, int timer )
     */
    timeout_data *td = (timeout_data *)t->data.v;
    if (--td->timer == 0) {
        handle_triggers(&td->triggers, NULL);
        return -1;
    }
    UNUSED_ARG(data);
    return 0;
}

static void timeout_write(const trigger * t, struct storage *store)
{
    timeout_data *td = (timeout_data *)t->data.v;
    WRITE_INT(store, td->timer);
    write_triggers(store, td->triggers);
}

static int timeout_read(trigger * t, gamedata *data)
{
    timeout_data *td = (timeout_data *)t->data.v;
    READ_INT(data->store, &td->timer);
    read_triggers(data, &td->triggers);
    if (td->timer > 20) {
        trigger *tr = td->triggers;
        log_warning("there is a timeout lasting for another %d turns\n", td->timer);
        while (tr) {
            log_warning("  timeout triggers: %s\n", tr->type->name);
            tr = tr->next;
        }
    }
    if (td->triggers != NULL && td->timer > 0)
        return AT_READ_OK;
    return AT_READ_FAIL;
}

trigger_type tt_timeout = {
    "timeout",
    timeout_init,
    timeout_free,
    timeout_handle,
    timeout_write,
    timeout_read
};

trigger *trigger_timeout(int time, trigger * callbacks)
{
    trigger *t = t_new(&tt_timeout);
    timeout_data *td = (timeout_data *)t->data.v;
    td->triggers = callbacks;
    td->timer = time;
    return t;
}
