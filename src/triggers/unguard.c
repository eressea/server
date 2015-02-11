/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */
#include <platform.h>
#include <kernel/config.h>
#include "unguard.h"

/* kernel includes */
#include <util/attrib.h>
#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>

/* libc includes */
#include <stdlib.h>

static int unguard_handle(trigger * t, void *data)
{
    building *b = (building *)t->data.v;

    if (b) {
        fset(b, BLD_UNGUARDED);
    }
    else {
        log_error("could not perform unguard::handle()\n");
        return -1;
    }
    unused_arg(data);
    return 0;
}

static void unguard_write(const trigger * t, struct storage *store)
{
    write_building_reference((building *)t->data.v, store);
}

static int unguard_read(trigger * t, struct storage *store)
{
    int rb =
        read_reference(&t->data.v, store, read_building_reference,
        resolve_building);
    if (rb == 0 && !t->data.v) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

struct trigger_type tt_unguard = {
    "building",
    NULL,
    NULL,
    unguard_handle,
    unguard_write,
    unguard_read
};

trigger *trigger_unguard(building * b)
{
    trigger *t = t_new(&tt_unguard);
    t->data.v = (void *)b;
    return t;
}
