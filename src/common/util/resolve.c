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
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include <assert.h>
#include <stdlib.h>
#include "resolve.h"

typedef struct unresolved {
	struct unresolved * next;
	void ** ptrptr;
		/* pointer to the location where the unresolved object
		 * should be, or NULL if special handling is required */
	void * data;
		/* information on how to resolve the missing object */
	resolve_fun resolve;
		/* function to resolve the unknown object */
} unresolved;

unresolved * ur_list;

void
ur_add(void * data, void ** ptrptr, resolve_fun fun) {
   unresolved * ur = malloc(sizeof(unresolved));
   ur->data = data;
   ur->resolve = fun;
   ur->ptrptr = ptrptr;
   ur->next = ur_list;
   ur_list = ur;
}

void
resolve(void)
{
	while (ur_list) {
		unresolved * ur = ur_list;
		ur_list = ur->next;
		if (ur->ptrptr) *ur->ptrptr = ur->resolve(ur->data);
		else ur->resolve(ur->data);
		free(ur);
	}
}
