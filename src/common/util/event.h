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

#ifndef EVENT_H
#define EVENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "variant.h"

struct attrib;
struct trigger;

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

typedef struct event_arg {
  char * type;
  void * data;
} event_arg;

extern trigger * t_new(trigger_type * ttype);
extern void t_free(trigger * t);
extern void t_add(trigger ** tlist, trigger * t);
/** add and handle triggers **/

/* add a trigger to a list of attributes */
extern void add_trigger(struct attrib ** ap, const char * eventname, struct trigger * t);
extern void remove_triggers(struct attrib ** ap, const char * eventname, const trigger_type * tt);
extern struct trigger ** get_triggers(struct attrib * ap, const char * eventname);
/* calls handle() for each of these. e.g. used in timeout */
extern void handle_event(struct attrib ** attribs, const char * eventname, void * data);
extern void handle_event_va(struct attrib ** attribs, const char * eventname, const char * format, ...);

/* functions for making complex triggers: */
extern void free_triggers(trigger * triggers); /* release all these triggers */
extern void write_triggers(FILE * F, const trigger * t);
extern void read_triggers(FILE * F, trigger ** tp);
extern int handle_triggers(trigger ** triggers, void * data);

extern struct attrib_type at_eventhandler;

#ifdef __cplusplus
}
#endif
#endif
