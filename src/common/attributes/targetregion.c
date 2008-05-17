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
#include "targetregion.h"

#include <kernel/eressea.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/version.h>

#include <util/attrib.h>
#include <util/resolve.h>
#include <util/storage.h>

static void
write_targetregion(const attrib * a, struct storage * store)
{
  write_region_reference((region*)a->data.v, store);
}

static int
read_targetregion(attrib * a, struct storage * store)
{
  region * r;
  int result = read_region_reference(&r, store);
  a->data.v = r;
  return result;
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
