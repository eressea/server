/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "changefaction.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/save.h>
#include <kernel/faction.h> /* FIXME: resolve_faction */

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/base36.h>
#include <util/storage.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#include <stdio.h>
/***
 ** restore a mage that was turned into a toad
 **/

typedef struct changefaction_data {
	struct unit * unit;
	struct faction * faction;
} changefaction_data;

static void
changefaction_init(trigger * t)
{
	t->data.v = calloc(sizeof(changefaction_data), 1);
}

static void
changefaction_free(trigger * t)
{
	free(t->data.v);
}

static int
changefaction_handle(trigger * t, void * data)
{
	/* call an event handler on changefaction.
	 * data.v -> ( variant event, int timer )
	 */
	changefaction_data * td = (changefaction_data*)t->data.v;
	if (td->unit && td->faction) {
		u_setfaction(td->unit, td->faction);
	} else {
		log_error(("could not perform changefaction::handle()\n"));
	}
	unused(data);
	return 0;
}

static void
changefaction_write(const trigger * t, struct storage * store)
{
	changefaction_data * td = (changefaction_data*)t->data.v;
	write_unit_reference(td->unit, store);
	write_faction_reference(td->faction, store);
}

static int
changefaction_read(trigger * t, struct storage * store)
{
	changefaction_data * td = (changefaction_data*)t->data.v;

	int u = read_unit_reference(&td->unit, store);
	int f = read_faction_reference(&td->faction, store);

	if (u!=AT_READ_OK || f!=AT_READ_OK) return AT_READ_FAIL;
	return AT_READ_OK;
}

trigger_type tt_changefaction = {
	"changefaction",
	changefaction_init,
	changefaction_free,
	changefaction_handle,
	changefaction_write,
	changefaction_read
};

trigger *
trigger_changefaction(unit * u, struct faction * f)
{
	trigger * t = t_new(&tt_changefaction);
	changefaction_data * td = (changefaction_data*)t->data.v;
	td->unit = u;
	td->faction = f;
	return t;
}
