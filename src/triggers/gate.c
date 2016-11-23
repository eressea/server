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
#include "gate.h"

 /* kernel includes */
#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/log.h>
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
    unused_arg(data);
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

    int bc =
        read_reference(&gd->gate, data, read_building_reference, resolve_building);
    int rc =
        read_reference(&gd->target, data, read_region_reference,
            RESOLVE_REGION(data->version));

    if (bc == 0 && rc == 0) {
        if (!gd->gate || !gd->target)
            return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

static void gate_init(trigger * t)
{
    t->data.v = calloc(sizeof(gate_data), 1);
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

trigger *trigger_gate(building * b, region * target)
{
    trigger *t = t_new(&tt_gate);
    gate_data *td = (gate_data *)t->data.v;
    td->gate = b;
    td->target = target;
    return t;
}
