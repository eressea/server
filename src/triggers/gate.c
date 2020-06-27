#include <platform.h>
#include "gate.h"

 /* kernel includes */
#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <kernel/attrib.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/resolve.h>

#include <storage.h>

/* libc includes */
#include <stdlib.h>

typedef struct gate_data {
    struct building *gate;
    struct region *target;
} gate_data;

static int gate_handle(trigger * t, void *data)
{
    /* call an event handler on gate.
     * data.v -> ( variant event, int timer )
     */
    gate_data *gd = (gate_data *)t->data.v;
    struct building *b = gd->gate;
    struct region *r = gd->target;

    if (b && b->region && r) {
        unit **up = &b->region->units;
        while (*up) {
            unit *u = *up;
            if (u->building == b)
                move_unit(u, r, NULL);
            if (*up == u)
                up = &u->next;
        }
    }
    else {
        log_error("could not perform gate::handle()\n");
        return -1;
    }
    UNUSED_ARG(data);
    return 0;
}

static void gate_write(const trigger * t, struct storage *store)
{
    gate_data *gd = (gate_data *)t->data.v;
    building *b = gd->gate;
    region *r = gd->target;

    write_building_reference(b, store);
    write_region_reference(r, store);
}

static int gate_read(trigger * t, gamedata *data)
{
    gate_data *gd = (gate_data *)t->data.v;
    int bc = read_building_reference(data, &gd->gate);
    int rc = read_region_reference(data, &gd->target);

    if (bc <= 0 && rc <= 0) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

static void gate_init(trigger * t)
{
    t->data.v = calloc(1, sizeof(gate_data));
}

static void gate_done(trigger * t)
{
    free(t->data.v);
}

struct trigger_type tt_gate = {
    "gate",
    gate_init,
    gate_done,
    gate_handle,
    gate_write,
    gate_read
};

