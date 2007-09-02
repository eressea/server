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
#include <eressea.h>
#include "killunit.h"

#include <unit.h>

/* util includes */
#include <util/base36.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>

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
  } else {
		log_warning(("could not perform killunit::handle()\n"));
  }
	unused(data);
	return 0;
}

static void
killunit_write(const trigger * t, FILE * F)
{
	unit * u = (unit*)t->data.v;
	write_unit_reference(u, F);
}

static int
killunit_read(trigger * t, FILE * F)
{
	return read_unit_reference((unit**)&t->data.v, F);
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
