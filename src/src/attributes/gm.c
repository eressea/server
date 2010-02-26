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
#include <kernel/config.h>
#include "gm.h"

/* kernel includes */
#include <kernel/plane.h>

/* util includes */
#include <util/attrib.h>
#include <util/storage.h>

static void
write_gm(const attrib * a, const void * owner, struct storage * store)
{
  write_plane_reference((plane*)a->data.v, store);
}

static int
read_gm(attrib * a, void * owner, struct storage * store)
{
  plane * pl;
  int result = read_plane_reference(&pl, store);
  a->data.v = pl;
  return result;
}


attrib_type at_gm = {
	"gm",
	NULL,
	NULL,
	NULL,
	write_gm,
	read_gm,
};

attrib *
make_gm(const struct plane * pl)
{
	attrib * a = a_new(&at_gm);
	a->data.v = (void*)pl;
	return a;
}
