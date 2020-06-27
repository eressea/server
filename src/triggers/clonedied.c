#include <platform.h>
#include "clonedied.h"

#include "magic.h"

/* kernel includes */
#include <kernel/spell.h>
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

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**
  clonedied.

  This trigger ist called when a clone of a mage dies.
  It simply removes the clone-attribute from the mage.
  */

static int clonedied_handle(trigger * t, void *data)
{
    /* destroy the unit */
    unit *u = (unit *)t->data.v;
    if (u) {
        attrib *a = a_find(u->attribs, &at_clone);
        if (a)
            a_remove(&u->attribs, a);
    }
    else
        log_error("could not perform clonedied::handle()\n");
    UNUSED_ARG(data);
    return 0;
}

static void clonedied_write(const trigger * t, struct storage *store)
{
    unit *u = (unit *)t->data.v;
    write_unit_reference(u, store);
}

static int clonedied_read(trigger * t, gamedata *data)
{
    if (read_unit_reference(data, (unit **)&t->data.v, NULL) <= 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

trigger_type tt_clonedied = {
    "clonedied",
    NULL,
    NULL,
    clonedied_handle,
    clonedied_write,
    clonedied_read
};

trigger *trigger_clonedied(unit * u)
{
    trigger *t = t_new(&tt_clonedied);
    t->data.v = (void *)u;
    return t;
}
