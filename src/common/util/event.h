/* vi: set ts=2:
 *
 *	$Id: event.h,v 1.3 2001/02/10 10:40:12 enno Exp $
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

#ifndef EVENT_H
#define EVENT_H

#include <stdio.h>

struct attrib;
struct trigger;

typedef union {
	void *v;
	int   i;
	char  c;
	short s;
	short sa[2];
	char  ca[4];
} variant;

typedef struct trigger_type {
	const char * name;
	void (*initialize)(struct trigger *);
	void (*finalize)(struct trigger *);
	int (*handle)(struct trigger *, void *);
	void (*write)(const struct trigger *, FILE * F);
	int (*read)(struct trigger *, FILE * F);

	struct trigger_type * next;
} trigger_type;

extern trigger_type * tt_find(const char * name);
extern void tt_register(trigger_type * ttype);

typedef struct trigger {
	struct trigger_type * type;
	struct trigger * next;

	variant data;
} trigger;

extern trigger * t_new(trigger_type * ttype);
extern void t_free(trigger * t);
extern void t_add(trigger ** tlist, trigger * t);
/** add and handle triggers **/

/* add a trigger to a list of attributes */
extern void add_trigger(struct attrib ** ap, const char * event, trigger * t);
/* calls handle() for each of these. e.g. used in timeout */
extern void handle_event(struct attrib ** attribs, const char * event, void * data);

/* functions for making complex triggers: */
extern void free_triggers(trigger * triggers); /* release all these triggers */
extern void write_triggers(FILE * F, const trigger * t);
extern void read_triggers(FILE * F, trigger ** tp);
extern int handle_triggers(trigger ** triggers, void * data);

extern struct attrib_type at_eventhandler;

#endif
