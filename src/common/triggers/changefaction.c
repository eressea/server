/* vi: set ts=2:
 *
 *	$Id: changefaction.c,v 1.4 2001/02/18 12:11:32 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "changefaction.h"

/* kernel includes */
#include <unit.h>
#include <save.h>
#include <faction.h> /* FIXME: resolve_faction */

/* util includes */
#include <resolve.h>
#include <event.h>
#include <base36.h>

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
changefaction_write(const trigger * t, FILE * F)
{
	changefaction_data * td = (changefaction_data*)t->data.v;
	write_unit_reference(td->unit, F);
	write_faction_reference(td->faction, F);
}

static int
changefaction_read(trigger * t, FILE * F)
{
	changefaction_data * td = (changefaction_data*)t->data.v;

	read_unit_reference(&td->unit, F);
	read_faction_reference(&td->faction, F);

	return 1;
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
