/* vi: set ts=2:
 *
 *	$Id: changerace.c,v 1.3 2001/04/12 17:21:45 enno Exp $
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
#include "changerace.h"

/* kernel includes */
#include <unit.h>

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
	race_t race;
	race_t irace;
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
		if (td->race!=NORACE) td->u->race = td->race;
		if (td->irace!=NORACE) td->u->irace = td->irace;
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
	fprintf(F, "%s ", itoa36(td->u->no));
	fprintf(F, "%d %d ", td->race, td->irace);
}

static int
changerace_read(trigger * t, FILE * F)
{
	changerace_data * td = (changerace_data*)t->data.v;
	char zText[128];
	int i;

	fscanf(F, "%s", zText);
	i = atoi36(zText);
	td->u = findunit(i);
	if (td->u==NULL) ur_add((void*)i, (void**)&td->u, resolve_unit);

	fscanf(F, "%d %d", &td->race, &td->irace);
	return 1;
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
trigger_changerace(unit * u, race_t race, race_t irace)
{
	trigger * t = t_new(&tt_changerace);
	changerace_data * td = (changerace_data*)t->data.v;
	td->u = u;
	td->race = race;
	td->irace = irace;
	return t;
}
