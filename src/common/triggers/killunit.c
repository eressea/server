/* vi: set ts=2:
 *
 *	$Id: killunit.c,v 1.2 2001/01/26 16:19:41 enno Exp $
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
#include "killunit.h"

#include <unit.h>

/* util includes */
#include <event.h>
#include <resolve.h>
#include <base36.h>

#include <stdio.h>
#include <stdlib.h>
/***
 ** killunit
 **/

static int
killunit_handle(trigger * t, void * data)
{
	/* call an event handler on killunit.
	 * data.v -> ( variant event, int timer )
	 */
	unit * u = (unit*)t->data.v;
	if (u!=NULL) {
		destroy_unit(u);
	} else
		fprintf(stderr, "\aERROR: could not perform killunit::handle()\n");
	unused(data);
	return 0;
}

static void
killunit_write(const trigger * t, FILE * F)
{
	unit * u = (unit*)t->data.v;
	fprintf(F, "%s ", itoa36(u->no));
}

static int
killunit_read(trigger * t, FILE * F)
{
	char zId[10];
	int i;
	fscanf(F, "%s", zId);
	i = atoi36(zId);
	t->data.v = findunit(i);
	if (t->data.v==NULL) ur_add((void*)i, &t->data.v, resolve_unit);
	return 1;
}

trigger_type tt_killunit = {
	"killunit",
	NULL,
	NULL,
	killunit_handle,
	killunit_write,
	killunit_read
};

trigger * 
trigger_killunit(unit * u)
{
	trigger * t = t_new(&tt_killunit);
	t->data.v = (void*)u;
	return t;
}
