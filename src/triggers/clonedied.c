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
#include "clonedied.h"

#include "magic.h"

/* kernel includes */
#include <kernel/spell.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/base36.h>

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
    int result =
        read_reference(&t->data.v, data, read_unit_reference, resolve_unit);
    if (result == 0 && t->data.v == NULL) {
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
