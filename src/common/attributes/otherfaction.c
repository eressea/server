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
#include "otherfaction.h"

#include <attrib.h>

/*
 * simple attributes that do not yet have their own file 
 */

attrib_type at_otherfaction = {
	"otherfaction", NULL, NULL, NULL, a_writedefault, a_readdefault, ATF_UNIQUE
};

void
init_otherfaction(void)
{
	at_register(&at_otherfaction);
}
