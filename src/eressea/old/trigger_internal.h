/* vi: set ts=2:
 *
 *	$Id: trigger_internal.h,v 1.1 2001/01/27 18:15:32 enno Exp $
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

#ifndef TRIGGER_INTERNAL_H
#define TRIGGER_INTERNAL_H

#ifndef OBJTYPES_H
# include <old/objtypes.h>
#endif
typedef enum {
	AC_NONE,			/* wird intern verwendet, nicht benutzen! */
	AC_DESTROY,
	AC_REMOVECURSE,
	AC_REMOVERELATION,
	AC_SENDMESSAGE,
	AC_CHANGERACE,
	AC_SHOCK,
	AC_CHANGEFACTION,
	AC_CREATEUNIT,
	AC_CHANGEIRACE,
	AC_CREATEMAGICBOOSTCURSE,

	MAX_ACTION_T
} action_t;

#define ACTION_MAGIC	0xC0DEBABE
typedef struct action {
	int magic;
	struct action *next;	/* Link in globaler action-List */
	int resid;				/* temporäre resolve-id */
	attrib *attribs;		/* für pointertags */

	action_t atype;

	void *obj;		/* points to self */
	typ_t typ;
	spread_t spread;

	/* arguments */
#define ACTION_INTS 4
	int i[ACTION_INTS];
	char *string;
} action;

typedef struct actionlist {
	struct actionlist *next;
	action *act;
} actionlist;

typedef struct old_trigger {
	struct old_trigger *next;	/* Link in globaler old_trigger-List */
	int resid;				/* temporäre resolve-id */
	attrib *attribs;		/* für pointertags */

	void *obj;				/* points to self */
	typ_t typ;

	trigger_t condition;
	spread_t spread;
	actionlist *acts;
} old_trigger;

typedef struct timeout {
	struct timeout *next;
	int resid;				/* temporäre resolve-id */
	attrib *attribs;		/* für pointertags */

	int ticks;
	actionlist *acts;
} timeout;

#endif /* TRIGGER_INTERNAL_H */
