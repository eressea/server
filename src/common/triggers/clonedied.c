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
#include "clonedied.h"

/* kernel includes */
#include <spell.h>
#include <magic.h>
#include <unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/resolve.h>
#include <util/base36.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**
  clonedied.
  
  This trigger ist called when a clone of a mage dies.
  It simply removes the clone-attribute from the mage.
 */

static int
clonedied_handle(trigger * t, void * data)
{
	/* destroy the unit */
	unit * u = (unit*)t->data.v;
	if (u!=NULL) {
		attrib *a = a_find(u->attribs, &at_clone);
		if(a) a_remove(&u->attribs, a);
	} else
		log_error(("could not perform clonedied::handle()\n"));
	unused(data);
	return 0;
}

static void
clonedied_write(const trigger * t, FILE * F)
{
	unit * u = (unit*)t->data.v;
	write_unit_reference(u, F);
}

static int
clonedied_read(trigger * t, FILE * F)
{
	return read_unit_reference((unit**)&t->data.v, F);
}

trigger_type tt_clonedied = {
	"clonedied",
	NULL,
	NULL,
	clonedied_handle,
	clonedied_write,
	clonedied_read
};

trigger *
trigger_clonedied(unit * u)
{
	trigger * t = t_new(&tt_clonedied);
	t->data.v = (void*)u;
	return t;
}

