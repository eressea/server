/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2005
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/types.h>
#include "alliance.h"

#include <kernel/save.h>
#include <util/attrib.h>

attrib_type at_alliance = {
	"alliance",
	NULL,
	NULL,
	NULL,
	a_writeint,
	a_readint,
	ATF_UNIQUE
};

void
init_alliance(void)
{
	at_register(&at_alliance);
}
