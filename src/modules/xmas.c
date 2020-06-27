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
#include <kernel/attrib.h>
#include <util/base36.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>
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
    if (read_building_reference(data, (building **)&t->data.v) <= 0) {
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

void register_xmas(void)
{
    tt_register(&tt_xmasgate);
}
