/* vi: set ts=2:
 *
 *	$Id: createunit.c,v 1.2 2001/01/26 16:19:41 enno Exp $
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
	race_t race;
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
		fprintf(stderr, "\aERROR: could not perform createunit::handle()\n");
	}
	unused(data);
	return 0;
}

static void
createunit_write(const trigger * t, FILE * F)
{
	createunit_data * td = (createunit_data*)t->data.v;
	fprintf(F, "%s ", itoa36(td->u->no));
	fprintf(F, "%d %d ", td->r->x, td->r->y);
	fprintf(F, "%d %d ", td->race, td->number);
}

static int
createunit_read(trigger * t, FILE * F)
{
	createunit_data * td = (createunit_data*)t->data.v;

	read_unit_reference(&td->u, F);
	read_region_reference(&td->r, F);

	fscanf(F, "%d %d ", &td->race, &td->number);
	return 1;
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
trigger_createunit(region * r, struct faction * f, race_t race, int number)
{
	trigger * t = t_new(&tt_createunit);
	createunit_data * td = (createunit_data*)t->data.v;
	td->r = r;
	td->f = f;
	td->race = race;
	td->number = number;
	return t;
}
