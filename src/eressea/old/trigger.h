/* vi: set ts=2:
 *
 *	$Id: trigger.h,v 1.2 2001/02/03 13:45:34 enno Exp $
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

#if !(defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER))
# error "Do not include unless for old code or to enable conversion"
#endif

#ifndef TRIGGER_H
#define TRIGGER_H

#include "relation.h"
#include "attrspread.h"

/* Ausloeser/Conditions */
typedef enum {
	TR_NONE,			/* wird intern benutzt, nicht verwenden */
	TR_DESTRUCT,	/* wenn das Objekt stirbt/zerstört wird */

	MAX_TRIGGER_T	/* must be last */
} trigger_t;

#include "trigger_internal.h"


/* old_trigger functions */

old_trigger *create_trigger(void *obj1, typ_t typ1, spread_t spread1,
			trigger_t condition);
void remove_trigger(old_trigger *t);

void do_trigger(void *obj1, typ_t typ1, trigger_t condition);


/* timeout functions */
#include "pointertags.h"
timeout *create_timeout(int ticks);
void remove_timeout(timeout *t);
void save_timeout_pointer(FILE *f, timeout *t, tag_t tag);

void save_timeouts(FILE *f);
void load_timeouts(FILE *f);
void countdown_timeouts(void);


/* link */

void link_action_trigger(action *a, old_trigger *t);
void link_action_timeout(action *a, timeout *t);


/* action functions */

void remove_action(action *a);
void remove_all_actions(void *obj, typ_t typ);
void save_action_pointer(FILE *f, action *a, tag_t tag);
int load_action_pointer(FILE *f, action **aP);


action *action_destroy(void *obj2, typ_t typ2, spread_t spread2);
action *action_removecurse(void *obj2, typ_t typ2, spread_t spread2,
			curse_t id, int id2);
action *action_removerelation(void *obj2, typ_t typ2, spread_t spread2,
			relation_t id);
action *action_sendmessage(void *obj2, typ_t typ2, spread_t spread2,
			char *m, msg_t mtype, int mlevel);
action *action_changeirace(void *obj2, typ_t typ2, spread_t spread2,
		race_t race);
action *action_changerace(void *obj2, typ_t typ2, spread_t spread2,
		race_t race, race_t irace);
action *action_shock(void *obj2, typ_t typ2, spread_t spread2);
action *action_changefaction(void *obj2, typ_t typ2, spread_t spread2,
		int unique_faction_id);
struct action * action_createunit(void *obj2, typ_t typ2, spread_t spr2,
		    int fno, int number, race_t race);
struct action * action_createmagicboostcurse(void *obj2, typ_t typ2, spread_t spr2,
				int power);

#include "attrib.h"

extern attrib_type at_trigger;
extern attrib_type at_action;

#endif /* TRIGGER_H */
