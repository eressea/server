/* vi: set ts=2:
 *
 *	$Id: shock.c,v 1.3 2001/02/15 02:41:47 enno Exp $
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
#include "shock.h"

/* kernel includes */
#include <spell.h>
#include <unit.h>

/* util includes */
#include <event.h>
#include <resolve.h>
#include <base36.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
/***
 ** shock
 **/

static int
shock_handle(trigger * t, void * data)
{
	/* destroy the unit */
	unit * u = (unit*)t->data.v;
	if (u!=NULL) {
		do_shock(u, "trigger");
	} else
		fprintf(stderr, "\aERROR: could not perform shock::handle()\n");
	unused(data);
	return 0;
}

static void
shock_write(const trigger * t, FILE * F)
{
	unit * u = (unit*)t->data.v;
	write_unit_reference(u, F);
}

static int
shock_read(trigger * t, FILE * F)
{
	read_unit_reference((unit**)&t->data.v, F);
	return 1;
}

trigger_type tt_shock = {
	"shock",
	NULL,
	NULL,
	shock_handle,
	shock_write,
	shock_read
};

trigger *
trigger_shock(unit * u)
{
	trigger * t = t_new(&tt_shock);
	t->data.v = (void*)u;
	return t;
}
