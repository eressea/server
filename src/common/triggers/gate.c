/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/
#include <config.h>
#include <eressea.h>
#include "gate.h"

/* kernel includes */
#include <building.h>
#include <region.h>
#include <unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/log.h>

/* libc includes */
#include <stdlib.h>

typedef struct gate_data {
	struct building * gate;
	struct region * target;
} gate_data;

static int
gate_handle(trigger * t, void * data)
{
	/* call an event handler on gate.
	 * data.v -> ( variant event, int timer )
	 */
	gate_data * gd = (gate_data*)t->data.v;
	struct building * b = gd->gate;
	struct region * r = gd->target;

	if (b && r) {
		unit ** up = &b->region->units;
		while (*up) {
			unit * u = *up;
			if (u->building==b) move_unit(u, r, NULL);
			if (*up==u) up = &u->next;
		}
	} else {
		log_error(("could not perform gate::handle()\n"));
		return -1;
	}
	unused(data);
	return 0;
}

static void
gate_write(const trigger * t, FILE * F)
{
	gate_data * gd = (gate_data*)t->data.v;
	building * b = gd->gate;
	region * r = gd->target;

	write_building_reference(b, F);
	write_region_reference(r, F);
}

static int
gate_read(trigger * t, FILE * F)
{
	gate_data * gd = (gate_data*)t->data.v;

	int bc = read_building_reference(&gd->gate, F);
	int rc = read_region_reference(&gd->target, F);

	if (rc!=AT_READ_OK || bc!=AT_READ_OK) return AT_READ_FAIL;
	return AT_READ_OK;
}

static void
gate_init(trigger * t)
{
	t->data.v = calloc(sizeof(gate_data), 1);
}

static void
gate_done(trigger * t)
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

trigger *
trigger_gate(building * b, region * target)
{
	trigger * t = t_new(&tt_gate);
	gate_data * td = (gate_data*)t->data.v;
	td->gate = b;
	td->target = target;
	return t;
}
