/* vi: set ts=2:
 *
 * $Id: key.c,v 1.2 2001/01/26 16:19:39 enno Exp $
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
make_key(int key)
{
	attrib * a = a_new(&at_key);
	a->data.i = key;
	return a;
}

void
init_key(void)
{
	at_register(&at_key);
}
