/* vi: set ts=2:
 *
 * $Id: targetregion.c,v 1.1 2001/01/25 09:37:57 enno Exp $
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
#include "targetregion.h"

#include <eressea.h>
#include <region.h>

#include <attrib.h>
#include <resolve.h>

static void
write_targetregion(const attrib * a, FILE * F)
{
	write_region_reference((region*)a->data.v, F);
}

static int
read_targetregion(attrib * a, FILE * F)
{
	if (global.data_version < BASE36IDS_VERSION) {
		a_readdefault(a, F);
		a->data.v = findregion(a->data.sa[0], a->data.sa[1]);
	} else {
		read_region_reference((region**)&a->data.v, F);
	}
	return 1;
}

attrib_type at_targetregion = {
	"targetregions",
	NULL,
	NULL,
	NULL,
	write_targetregion,
	read_targetregion,
	ATF_UNIQUE
};

attrib *
make_targetregion(struct region * r)
{
	attrib * a = a_new(&at_targetregion);
	a->data.v = r;
	return a;
}

void
init_targetregion(void)
{
	at_register(&at_targetregion);
}
