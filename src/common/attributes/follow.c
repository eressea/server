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
#include "follow.h"

#include <eressea.h>
#include <unit.h>

#include <attrib.h>
#include <resolve.h>

static int
verify_follow(attrib * a)
{
	if (a->data.v==NULL) {
		return 0;
	}
	return 1;
}

#define FOLLOW_PERSISTENT 0

#if FOLLOW_PERSISTENT
static void
write_follow(const attrib * a, FILE * F)
{
	write_unit_reference((unit*)a->data.v, F);
}
#else
#define write_follow NULL
#endif

static int
read_follow(attrib * a, FILE * F)
{
	if (global.data_version < BASE36IDS_VERSION) {
		int i;
		fscanf(F, "%d", &i);
#if FOLLOW_PERSISTENT
		ur_add((void*)i, (void**)&a->data.v, resolve_unit);
#endif
	} else {
#if FOLLOW_PERSISTENT
		read_unit_reference((unit**)&a->data.v, F);
#else
		read_unit_reference(NULL, F);
#endif
	}
	return FOLLOW_PERSISTENT;
}

attrib_type at_follow = {
	"follow", NULL, NULL, verify_follow, write_follow, read_follow
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
