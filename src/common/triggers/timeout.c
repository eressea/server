/* vi: set ts=2:
 *
 *	$Id: timeout.c,v 1.1 2001/01/25 09:37:57 enno Exp $
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
#include "timeout.h"

#include <event.h>

#include <stdio.h>
#include <stdlib.h>
/***
 ** timeout
 **/

typedef struct timeout_data {
	trigger * triggers;
	int timer;
	variant trigger_data;
} timeout_data;

static void
timeout_init(trigger * t)
{
	t->data.v = calloc(sizeof(timeout_data), 1);
}

static void
timeout_free(trigger * t)
{
	timeout_data * td = (timeout_data*)t->data.v;
	free_triggers(td->triggers);
	free(t->data.v);
}

static int
timeout_handle(trigger * t, void * data)
{
	/* call an event handler on timeout.
	 * data.v -> ( variant event, int timer )
	 */
	timeout_data * td = (timeout_data*)t->data.v;
	if (--td->timer==0) {
		handle_triggers(&td->triggers, NULL);
	}
	unused(data);
	return td->timer;
}

static void
timeout_write(const trigger * t, FILE * F)
{
	timeout_data * td = (timeout_data*)t->data.v;
	fprintf(F, "%d ", td->timer);
	write_triggers(F, td->triggers);
}

static int
timeout_read(trigger * t, FILE * F)
{
	timeout_data * td = (timeout_data*)t->data.v;
	fscanf(F, "%d", &td->timer);
	read_triggers(F, &td->triggers);
	return (td->triggers!=NULL && td->timer>0);
}

trigger_type tt_timeout = {
	"timeout",
	timeout_init,
	timeout_free,
	timeout_handle,
	timeout_write,
	timeout_read
};

trigger *
trigger_timeout(int time, trigger * callbacks)
{
	trigger * t = t_new(&tt_timeout);
	timeout_data * td = (timeout_data*)t->data.v;
	td->triggers = callbacks;
	td->timer = time;
	return t;
}
