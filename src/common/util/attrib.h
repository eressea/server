/* vi: set ts=2:
 *
 *	$Id: attrib.h,v 1.2 2001/01/26 16:19:41 enno Exp $
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

#ifndef ATTRIB_H
#define ATTRIB_H

#include <stdio.h>

typedef void (*afun)(void);

typedef struct attrib {
	const struct attrib_type * type;
	union {
		afun f;
		void * v;
		int i;
		char c;
		short s;
		short sa[2];
		char ca[4];
	} data;
	/* internal data, do not modify: */
	struct attrib * next;
	struct attrib * nexttype;
} attrib;

#define ATF_UNIQUE   (1<<0) /* only one per attribute-list */
#define ATF_PRESERVE (1<<1) /* preserve order in list. append to back */
#define ATF_USER_DEFINED (1<<2) /* use this to make udf */

typedef struct attrib_type {
	const char* name;
	void (*initialize)(struct attrib *);
	void (*finalize)(struct attrib *);
	int  (*age)(struct attrib *);
	/* age returns 0 if the attribute needs to be removed, !=0 otherwise */
	void (*write)(const struct attrib *, FILE*);
	int  (*read)(struct attrib *, FILE*); /* return 1 on success, 0 if attrib needs removal */
	unsigned int flags;
	/* ---- internal data, do not modify: ---- */
	struct attrib_type * nexthash;
	unsigned int hashkey;
} attrib_type;

extern int a_readdefault(attrib * a, FILE * f);
extern void a_writedefault(const attrib * a, FILE * f);

extern int a_readstring(attrib * a, FILE * f);
extern void a_writestring(const attrib * a, FILE * f);
extern void a_finalizestring(attrib * a);

extern void at_register(attrib_type * at);

extern attrib * a_select(attrib * a, void * data, boolean(*compare)(const attrib *, void *));
extern attrib * a_find(attrib * a, const attrib_type * at);
extern attrib * a_add(attrib ** pa, attrib * at);
extern int a_remove(attrib ** pa, attrib * at);
extern void a_removeall(attrib ** a, const attrib_type * at);
extern attrib * a_new(const attrib_type * at);
extern void a_free(attrib * a);

extern int a_age(attrib ** attribs);
extern void a_read(FILE * f, attrib ** attribs);
extern void a_write(FILE * f, const attrib * attribs);

#define DEFAULT_AGE NULL
#define DEFAULT_INIT NULL
#define DEFAULT_FINALIZE NULL
#define DEFAULT_WRITE a_writedefault
#define DEFAULT_READ a_readdefault
#define NO_WRITE NULL
#define NO_READ NULL

#endif
