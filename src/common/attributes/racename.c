/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "racename.h"

#include <kernel/save.h>
#include <util/attrib.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

attrib_type at_racename = {
	"racename", NULL, a_finalizestring, NULL, a_writestring, a_readstring
};

const char * 
get_racename(attrib * alist)
{
	attrib * a = a_find(alist, &at_racename);
	if (a) return (const char *)a->data.v;
	return NULL;
}

void
set_racename(attrib ** palist, const char * name)
{
	attrib * a = a_find(*palist, &at_racename);
	if (!a && name) {
		a = a_add(palist, a_new(&at_racename));
		a->data.v = strdup(name);
	} else if (a && !name) {
		a_remove(palist, a);
	} else if (a) {
		if (strcmp(a->data.v, name)!=0) {
			free(a->data.v);
			a->data.v = strdup(name);
		}
	}
}

void
init_racename(void)
{
	at_register(&at_racename);
}
