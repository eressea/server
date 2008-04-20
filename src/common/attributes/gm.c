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
#include <kernel/eressea.h>
#include "gm.h"

/* kernel includes */
#include <kernel/plane.h>

/* util includes */
#include <util/attrib.h>

static void
write_gm(const attrib * a, FILE * F)
{
	write_plane_reference((plane*)a->data.v, F);
}

static int
read_gm(attrib * a, FILE * F)
{
	return read_plane_reference((plane**)&a->data.v, F);
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

void
init_gm(void)
{
	at_register(&at_gm);
}
