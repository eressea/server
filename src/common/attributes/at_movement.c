/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
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
#include <attrib.h>
#include "at_movement.h"

static void
write_movement(const attrib * a, FILE * F)
{
	fprintf(F, "%d", a->data.i);
}

static int
read_movement(attrib * a, FILE * F)
{
	fscanf(F, "%d", &a->data.i);
	if (a->data.i !=0 ) return AT_READ_OK;
	else return AT_READ_FAIL;
}

attrib_type at_movement = {
	"movement", NULL, NULL, NULL, write_movement, read_movement
};

boolean
get_movement(attrib ** alist, int type)
{
	attrib * a = a_find(*alist, &at_movement);
	if (a==NULL) return false;
	if (a->data.i & type) return true;
	return false;
}

void
set_movement(attrib ** alist, int type)
{
	attrib * a = a_find(*alist, &at_movement);
	if (a==NULL) a = a_add(alist, a_new(&at_movement));
	a->data.i |= type;
}

void
init_movement(void)
{
	at_register(&at_movement);
}
