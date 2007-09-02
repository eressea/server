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
#include <eressea.h>
#include "moved.h"

#include <attrib.h>

static int
age_moved(attrib * a)
{
	--a->data.i;
	return a->data.i > 0;
}

static void
write_moved(const attrib * a, FILE * F)
{
	fprintf(F, "%d ", a->data.i);
}

static int
read_moved(attrib * a, FILE * F)
{
	fscanf(F, "%d", &a->data.i);
	if (a->data.i !=0 ) return AT_READ_OK;
	else return AT_READ_FAIL;
}

attrib_type at_moved = {
	"moved", NULL, NULL, age_moved, write_moved, read_moved
};

boolean
get_moved(attrib ** alist)
{
	return a_find(*alist, &at_moved) ? true : false;
}

void
set_moved(attrib ** alist)
{
	attrib * a = a_find(*alist, &at_moved);
	if (a==NULL) a = a_add(alist, a_new(&at_moved));
	a->data.i = 2;
}

void
init_moved(void)
{
	at_register(&at_moved);
}
