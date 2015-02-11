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
#include "timeout.h"

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/log.h>

#include <storage.h>

#include <stdio.h>
#include <stdlib.h>
/***
 ** timeout
 **/

typedef struct timeout_data {
    trigger *triggers;
    int timer;
    variant trigger_data;
} timeout_data;

static void timeout_init(trigger * t)
{
    t->data.v = calloc(sizeof(timeout_data), 1);
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
    unused_arg(data);
    return 0;
}

static void timeout_write(const trigger * t, struct storage *store)
{
    timeout_data *td = (timeout_data *)t->data.v;
    WRITE_INT(store, td->timer);
    write_triggers(store, td->triggers);
}

static int timeout_read(trigger * t, struct storage *store)
{
    timeout_data *td = (timeout_data *)t->data.v;
    READ_INT(store, &td->timer);
    read_triggers(store, &td->triggers);
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
