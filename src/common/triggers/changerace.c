/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "changerace.h"

/* kernel includes */
#include <unit.h>
#include <race.h>

/* util includes */
#include <event.h>
#include <resolve.h>
#include <base36.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#include <stdio.h>
/***
 ** restore a mage that was turned into a toad
 **/

typedef struct changerace_data {
	struct unit * u;
	const struct race * race;
	const struct race * irace;
} changerace_data;

static void
changerace_init(trigger * t)
{
	t->data.v = calloc(sizeof(changerace_data), 1);
}

static void
changerace_free(trigger * t)
{
	free(t->data.v);
}

static int
changerace_handle(trigger * t, void * data)
{
	/* call an event handler on changerace.
	 * data.v -> ( variant event, int timer )
	 */
	changerace_data * td = (changerace_data*)t->data.v;
	if (td->u) {
		if (td->race!=NULL) td->u->race = td->race;
		if (td->irace!=NULL) td->u->irace = td->irace;
	} else {
		log_error(("could not perform changerace::handle()\n"));
	}
	unused(data);
	return 0;
}

static void
changerace_write(const trigger * t, FILE * F)
{
	changerace_data * td = (changerace_data*)t->data.v;
	write_unit_reference(td->u, F);
	write_race_reference(td->race, F);
	write_race_reference(td->irace, F);
}

static int
changerace_read(trigger * t, FILE * F)
{
	changerace_data * td = (changerace_data*)t->data.v;
	int uc = read_unit_reference(&td->u, F);
	int rc = read_race_reference(&td->race, F);
	int ic = read_race_reference(&td->irace, F);
	if (uc!=AT_READ_OK || rc!=AT_READ_OK || ic!=AT_READ_OK) return AT_READ_FAIL;
	return AT_READ_OK;
}

trigger_type tt_changerace = {
	"changerace",
	changerace_init,
	changerace_free,
	changerace_handle,
	changerace_write,
	changerace_read
};

trigger *
trigger_changerace(struct unit * u, const struct race * prace, const struct race * irace)
{
	trigger * t = t_new(&tt_changerace);
	changerace_data * td = (changerace_data*)t->data.v;
	td->u = u;
	td->race = prace;
	td->irace = irace;
	return t;
}
