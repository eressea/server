/* vi: set ts=2:
 *
 *	
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
#include "createunit.h"

/* kernel includes */
#include <unit.h>
#include <race.h>
#include <region.h>

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

typedef struct createunit_data {
	struct region * r;
	struct faction * f;
	struct unit * u;
	const struct race * race;
	int number;
} createunit_data;

static void
createunit_init(trigger * t)
{
	t->data.v = calloc(sizeof(createunit_data), 1);
}

static void
createunit_free(trigger * t)
{
	free(t->data.v);
}

static int
createunit_handle(trigger * t, void * data)
{
	/* call an event handler on createunit.
	 * data.v -> ( variant event, int timer )
	 */
	createunit_data * td = (createunit_data*)t->data.v;
	if (td->r!=NULL && td->f!=NULL) {
		createunit(td->r, td->f, td->number, td->race);
	} else {
		log_error(("could not perform createunit::handle()\n"));
	}
	unused(data);
	return 0;
}

static void
createunit_write(const trigger * t, FILE * F)
{
	createunit_data * td = (createunit_data*)t->data.v;
	write_unit_reference(td->u, F);
	write_region_reference(td->r, F);
	write_race_reference(td->race, F);
	fprintf(F, "%d ", td->number);
}

static int
createunit_read(trigger * t, FILE * F)
{
	createunit_data * td = (createunit_data*)t->data.v;

	int uc = read_unit_reference(&td->u, F);
	int rc = read_region_reference(&td->r, F);
	int ic = read_race_reference(&td->race, F);

	fscanf(F, "%d ", &td->number);

	if (uc!=AT_READ_OK || rc!=AT_READ_OK || ic!=AT_READ_OK) return AT_READ_FAIL;
	return AT_READ_OK;
}

trigger_type tt_createunit = {
	"createunit",
	createunit_init,
	createunit_free,
	createunit_handle,
	createunit_write,
	createunit_read
};

trigger *
trigger_createunit(region * r, struct faction * f, const struct race * rc, int number)
{
	trigger * t = t_new(&tt_createunit);
	createunit_data * td = (createunit_data*)t->data.v;
	td->r = r;
	td->f = f;
	td->race = rc;
	td->number = number;
	return t;
}
