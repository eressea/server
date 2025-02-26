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
        else
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

static struct trigger *change_race_i(struct unit *u, int duration, const struct race *rc, bool is_stealth) {
    trigger *tr = NULL, *tt = NULL;
    trigger **tp = get_triggers(u->attribs, "timer");
    if (tp)
    {
        while (*tp) {
            trigger *t = *tp;
            if (t->type == &tt_timeout) {
                timeout_data *td = (timeout_data *)t->data.v;
                for (tr = td->triggers; tr; tr = tr->next) {
                    if (tr->type == &tt_changerace) {
                        tt = t; /* return the old timeout, don't create a new one */
                        break;
                    }
                }
            }
            tp = &t->next;
        }
    }
    /* only add another change_race timeout if there wasn't one already (toad becomes smurf) */
    if (!tt) {
        if (is_stealth) {
            tr = trigger_changerace(u, NULL, u->irace);
        }
        else {
            tr = trigger_changerace(u, u_race(u), u->irace);
        }
        if (tr) {
            tt = trigger_timeout(duration, tr);
            add_trigger(&u->attribs, "timer", tt);
        }
    }
    if (tr) {
        if (is_stealth) {
            u->irace = rc;
        }
        else {
            u_setrace(u, rc);
            u->irace = NULL;
        }
    }
    return tt;
}

struct trigger* change_race(struct unit* u, int duration, const struct race* urace, const struct race* irace) {
    const struct race* rc = irace ? irace : urace;
    return change_race_i(u, duration, rc, irace != NULL);
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

trigger *get_change_race_trigger(unit *u)
{
    trigger **tp = get_triggers(u->attribs, "timer");
    if (tp) {
        while (*tp) {
            trigger *tr = *tp;
            if (tr->type == &tt_timeout) {
                timeout_data *td = (timeout_data *)tr->data.v;
                trigger *t;
                for (t = td->triggers; t; t = t->next) {
                    if (t->type == &tt_changerace) {
                        return t;
                    }
                }
            }
            tp = &tr->next;
        }
    }
    return NULL;
}

