#include <platform.h>
#include "killunit.h"

#include <kernel/region.h>
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

#include <stdio.h>
#include <stdlib.h>
/***
 ** killunit
 **/

static int killunit_handle(trigger * t, void *data)
{
    /* call an event handler on killunit.
     * data.v -> ( variant event, int timer )
     */
    unit *u = (unit *)t->data.v;
    if (u) {
        /* we can't remove_unit() here, because that's what's calling us. */
        set_number(u, 0);
    }
    UNUSED_ARG(data);
    return 0;
}

static void killunit_write(const trigger * t, struct storage *store)
{
    unit *u = (unit *)t->data.v;
    write_unit_reference(u, store);
}

static int killunit_read(trigger * t, gamedata *data)
{
    if (read_unit_reference(data, (unit **)&t->data.v, NULL) == 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

trigger_type tt_killunit = {
    "killunit",
    NULL,
    NULL,
    killunit_handle,
    killunit_write,
    killunit_read
};

trigger *trigger_killunit(unit * u)
{
    trigger *t = t_new(&tt_killunit);
    t->data.v = (void *)u;
    return t;
}
