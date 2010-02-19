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
#include "matmod.h"

#include <util/attrib.h>

attrib_type at_matmod = {
	"matmod",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ATF_PRESERVE
};

attrib *
make_matmod(mm_fun function)
{
	attrib * a = a_new(&at_matmod);
	a->data.f = (void(*)(void))function;
	return a;
}
