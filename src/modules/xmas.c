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
#include "xmas.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <move.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/goodies.h>
#include <util/resolve.h>

#include <storage.h>

/* libc includes */
#include <stdlib.h>

static int xmasgate_handle(trigger * t, void *data)
{
    return -1;
}

static void xmasgate_write(const trigger * t, struct storage *store)
{
    building *b = (building *)t->data.v;
    WRITE_TOK(store, itoa36(b->no));
}

static int xmasgate_read(trigger * t, struct gamedata *data)
{
    int bc =
        read_reference(&t->data.v, data->store, read_building_reference,
        resolve_building);
    if (bc == 0 && !t->data.v) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

struct trigger_type tt_xmasgate = {
    "xmasgate",
    NULL,
    NULL,
    xmasgate_handle,
    xmasgate_write,
    xmasgate_read
};

trigger *trigger_xmasgate(building * b)
{
    trigger *t = t_new(&tt_xmasgate);
    t->data.v = b;
    return t;
}

void register_xmas(void)
{
    tt_register(&tt_xmasgate);
}
