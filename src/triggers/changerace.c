#include "changerace.h"
#include "timeout.h"

/* kernel includes */
#include <kernel/faction.h>
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
    if (t) {
        changerace_data *td = (changerace_data *)t->data.v;

        td->u = u;
        td->race = prace;
        td->irace = irace;
    }
    return t;
}

struct trigger *change_race(struct unit *u, int duration, const struct race *urace, const struct race *irace) {
    trigger **texists = get_triggers(u->attribs, "timer");
    trigger *tr = NULL;

    if (texists) {
        tr = *texists;
    }
    else {
        trigger *trestore = trigger_changerace(u, u_race(u), u->irace);
        if (trestore) {
            tr = trigger_timeout(duration, trestore);
            add_trigger(&u->attribs, "timer", tr);
        }
    }
    if (tr) {
        u->irace = irace;
        if (urace) {
            u_setrace(u, urace);
        }
    }
    return tr;
}

void restore_race(unit *u, const race *rc)
{
    if (u->attribs) {
        trigger **tp = get_triggers(u->attribs, "timer");
        if (tp) {
            while (*tp) {
                trigger *t = *tp;
                if (t->type == &tt_timeout) {
                    timeout_data *td = (timeout_data *)t->data.v;
                    trigger *tr = td->triggers;
                    for (; tr; tr = tr->next) {
                        if (tr->type != &tt_changerace) {
                            break;
                        }
                    }
                    if (tr == NULL) {
                        *tp = t->next;
                        t_free(t);
                        continue;
                    }
                }
                tp = &t->next;
            }
        }
    }
    u->_race = rc;
}

