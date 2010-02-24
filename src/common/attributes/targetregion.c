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

#include <platform.h>
#include "targetregion.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/version.h>

#include <util/attrib.h>
#include <util/resolve.h>
#include <util/storage.h>

static void
write_targetregion(const attrib * a, const void * owner, struct storage * store)
{
  write_region_reference((region*)a->data.v, store);
}

static int
read_targetregion(attrib * a, void * owner, struct storage * store)
{
  int result = read_reference(&a->data.v, store, read_region_reference, RESOLVE_REGION(store->version));
  if (result==0 && !a->data.v) return AT_READ_FAIL;
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
