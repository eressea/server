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
#include "targetregion.h"

#include <kernel/eressea.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/version.h>

#include <util/attrib.h>
#include <util/resolve.h>

static void
write_targetregion(const attrib * a, FILE * F)
{
	write_region_reference((region*)a->data.v, F);
}

static int
read_targetregion(attrib * a, FILE * F)
{
	if (global.data_version < BASE36IDS_VERSION) {
		a_readint(a, F);
		a->data.v = findregion(a->data.sa[0], a->data.sa[1]);
	} else {
		return read_region_reference((region**)&a->data.v, F);
	}
	return AT_READ_OK;
}

attrib_type at_targetregion = {
	"targetregion",
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
