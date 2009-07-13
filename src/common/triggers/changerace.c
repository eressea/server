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
#include "changerace.h"

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/race.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/storage.h>
#include <util/base36.h>

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
changerace_write(const trigger * t, struct storage * store)
{
	changerace_data * td = (changerace_data*)t->data.v;
	write_unit_reference(td->u, store);
	write_race_reference(td->race, store);
	write_race_reference(td->irace, store);
}

static int
changerace_read(trigger * t, struct storage * store)
{
  changerace_data * td = (changerace_data*)t->data.v;
  read_reference(&td->u, store, read_unit_reference, resolve_unit);
  td->race = (const struct race*)read_race_reference(store).v;
  td->irace = (const struct race*)read_race_reference(store).v;
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
  const race * uirace = u_irace(u);
  trigger * t = t_new(&tt_changerace);
  changerace_data * td = (changerace_data*)t->data.v;

  assert(u->race==uirace || "!changerace-triggers cannot stack!");
  td->u = u;
  td->race = prace;
  td->irace = irace;
  return t;
}
