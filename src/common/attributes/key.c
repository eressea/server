/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "key.h"

#include <attrib.h>

attrib_type at_key = {
	"key",
	NULL,
	NULL,
	NULL,
	a_writedefault,
	a_readdefault,
};

attrib * 
add_key(attrib ** alist, int key)
{
	attrib * a = find_key(*alist, key);
	if (a==NULL) a = a_add(alist, make_key(key));
	return a;
}

attrib *
make_key(int key)
{
	attrib * a = a_new(&at_key);
	a->data.i = key;
	return a;
}

attrib *
find_key(attrib * alist, int key)
{
	attrib * a = a_find(alist, &at_key);
	while (a && a->data.i != key) a = a->nexttype;
	return a;
}

void
init_key(void)
{
	at_register(&at_key);
}
