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
#include "eressea.h"
#include "iceberg.h"

#include <attrib.h>

attrib_type at_iceberg = {
	"iceberg_drift",
	NULL,
	NULL,
	NULL,
	a_writedefault,
	a_readdefault,
	ATF_UNIQUE
};

attrib *
make_iceberg(direction_t dir)
{
	attrib * a = a_new(&at_iceberg);
	a->data.i = (int)dir;
	return a;
}

void
init_iceberg(void)
{
	at_register(&at_iceberg);
}
