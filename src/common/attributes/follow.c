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
#include "follow.h"

#include <eressea.h>
#include <kernel/unit.h>
#include <kernel/version.h>

#include <attrib.h>
#include <resolve.h>
#include <variant.h>

static int
verify_follow(attrib * a)
{
	if (a->data.v==NULL) {
		return 0;
	}
	return 1;
}

static int
read_follow(attrib * a, FILE * F)
{
	return read_unit_reference(NULL, F);
}

attrib_type at_follow = {
	"follow", NULL, NULL, verify_follow, NULL, read_follow
};

attrib *
make_follow(struct unit * u)
{
	attrib * a = a_new(&at_follow);
	a->data.v = u;
	return a;
}

void
init_follow(void)
{
	at_register(&at_follow);
}
