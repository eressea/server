/* vi: set ts=2:
 *
 *	$Id: event.c,v 1.3 2001/02/03 13:45:33 enno Exp $
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
#include "event.h"

/* util includes */
#include "attrib.h"
#include "resolve.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void 
write_triggers(FILE * F, const trigger * t)
{
	while (t) {
		if (t->type->write) {
			fprintf(F, "%s ", t->type->name);
			t->type->write(t, F);
		}
		t = t->next;
	}
	fputs("end ", F);
}

void
read_triggers(FILE * F, trigger ** tp)
{
	for (;;) {
		trigger_type * ttype;
		char zText[128];
		fscanf(F, "%s", zText);
		if (!strcmp(zText, "end")) break;
		ttype = tt_find(zText);
		assert(ttype || !"unknown trigger-type");
		*tp = t_new(ttype);
		if (ttype->read) ttype->read(*tp, F);
		tp = &(*tp)->next;
	}
}

trigger *
t_new(trigger_type * ttype)
{
	trigger * t = calloc(sizeof(trigger), 1);
	t->type=ttype;
	if (ttype->initialize) ttype->initialize(t);
	return t;
}

void
t_free(trigger * t)
{
	if (t->type->finalize) t->type->finalize(t);
}

void
free_triggers(trigger * triggers) 
{
	while (triggers) {
		trigger * t = triggers;
		triggers = t->next;
		t_free(t);
	}
}

int
handle_triggers(trigger ** triggers, void * param)
{
	trigger ** tp = triggers;
	while (*tp) {
		trigger * t = *tp;
		if (t->type->handle(t, param)!=0) {
			*tp=t->next;
			t_free(t);
		} 
		else tp=&t->next;
	}
	return (*triggers!=NULL);
}

/***
 ** at_eventhandler
 **/

typedef struct handler_info {
	char * event;
	trigger * triggers;
} handler_info;

static void 
init_handler(attrib * a) {
	a->data.v = calloc(sizeof(handler_info), 1);
}

static void
free_handler(attrib * a) {
	handler_info *hi = (handler_info*)a->data.v;
	free_triggers(hi->triggers);
	free(hi->event);
	free(hi);
}

static void
write_handler(const attrib * a, FILE * F)
{
	handler_info *hi = (handler_info*)a->data.v;
	fprintf(F, "%s ", hi->event);
	write_triggers(F, hi->triggers);
}

static int
read_handler(attrib * a, FILE * F)
{
	char zText[128];
	handler_info *hi = (handler_info*)a->data.v;
	fscanf(F, "%s ", zText);
	hi->event = strdup(zText);
	read_triggers(F, &hi->triggers);
	return (hi->triggers!=NULL);
}

attrib_type at_eventhandler = {
	"eventhandler",
	init_handler,
	free_handler,
	NULL,
	write_handler,
	read_handler
};

void
add_trigger(attrib ** ap, const char * event, trigger * t)
{
	trigger ** tp;
	handler_info * td = NULL;
	attrib * a = a_find(*ap, &at_eventhandler);
	assert(t->next==NULL);
	while (a!=NULL) {
		td = (handler_info *)a->data.v;
		if (!strcmp(td->event, event)) {
			break;
		}
		a = a->next;
	}
	if (!a) {
		a = a_add(ap, a_new(&at_eventhandler));
		td = (handler_info *)a->data.v;
		td->event = strdup(event);
	}
	tp = &td->triggers;
	while (*tp) tp=&(*tp)->next;
	*tp = t;
}

void
handle_event(attrib ** attribs, const char * event, void * param)
{
	while (*attribs) {
		if ((*attribs)->type==&at_eventhandler) break;
		attribs = &(*attribs)->next;
	}
	while (*attribs) {
		handler_info * tl = (handler_info*)(*attribs)->data.v;
		if (!strcmp(tl->event, event)) break;
		attribs = &(*attribs)->nexttype;
	}
	if (*attribs) {
		handler_info * tl = (handler_info*)(*attribs)->data.v;
		handle_triggers(&tl->triggers, param);
	}
}

void
t_add(trigger ** tlist, trigger * t)
{
	while (*tlist) tlist = &(*tlist)->next;
	*tlist = t;
}

static trigger_type * triggertypes;

void
tt_register(trigger_type * tt) 
{
	tt->next = triggertypes;
	triggertypes = tt;
}

trigger_type *
tt_find(const char * name)
{
	trigger_type * tt = triggertypes;
	while (tt && strcmp(tt->name, name)) tt = tt->next;
	return tt;
}


/***
 ** default events
 **/

