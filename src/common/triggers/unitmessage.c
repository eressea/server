/* vi: set ts=2:
 *
 *	$Id: unitmessage.c,v 1.1 2001/01/25 09:37:57 enno Exp $
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
#include "unitmessage.h"

/* kernel includes */
#include <unit.h>

/* util includes */
#include <resolve.h>
#include <event.h>
#include <base36.h>
#include <goodies.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/***
 ** give an item to someone
 **/

typedef struct unitmessage_data {
	struct unit * target;
	char * string;
	int type;
	int level;
} unitmessage_data;

static void
unitmessage_init(trigger * t)
{
	t->data.v = calloc(sizeof(unitmessage_data), 1);
}

static void
unitmessage_free(trigger * t)
{
	unitmessage_data * sd = (unitmessage_data*)t->data.v;
	free(sd->string);
	free(t->data.v);
}

static int
unitmessage_handle(trigger * t, void * data)
{
	/* call an event handler on unitmessage.
	 * data.v -> ( variant event, int timer )
	 */
	unitmessage_data * td = (unitmessage_data*)t->data.v;
	if (td->target!=NULL) {
		addmessage(td->target->region, td->target->faction, td->string, td->type, td->level);
	} else
		fprintf(stderr, "\aERROR: could not perform unitmessage::handle()\n");
	unused(data);
	return 0;
}

static void
unitmessage_write(const trigger * t, FILE * F)
{
	unitmessage_data * td = (unitmessage_data*)t->data.v;
	fprintf(F, "%s ", itoa36(td->target->no));
	fprintf(F, "%s %d %d ", td->string, td->type, td->level);
}

static int
unitmessage_read(trigger * t, FILE * F)
{
	unitmessage_data * td = (unitmessage_data*)t->data.v;
	char zText[256];
	int i;

	fscanf(F, "%s", zText);
	i = atoi36(zText);
	td->target = findunit(i);
	if (td->target==NULL) ur_add((void*)i, (void**)&td->target, resolve_unit);

	fscanf(F, "%s %d %d ", zText, &td->type, &td->level);
	td->string = strdup(zText);

	return 1;
}

trigger_type tt_unitmessage = {
	"unitmessage",
	unitmessage_init,
	unitmessage_free,
	unitmessage_handle,
	unitmessage_write,
	unitmessage_read
};

trigger *
trigger_unitmessage(unit * target, const char * string, int type, int level)
{
	trigger * t = t_new(&tt_unitmessage);
	unitmessage_data * td = (unitmessage_data*)t->data.v;
	td->target = target;
	td->string = escape_string(strdup(string), SPACE_REPLACEMENT);
	td->type = type;
	td->level = level;
	return t;
}
