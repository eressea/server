/* vi: set ts=2:
 *
 * $Id: attributes.c,v 1.3 2001/02/02 08:40:48 enno Exp $
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
#include <eressea.h>
#include "attributes.h"

/* attributes includes */
#include <attributes/key.h>
#include <attributes/orcification.h>
#include <attributes/targetregion.h>
#include <attributes/reduceproduction.h>
#include <attributes/follow.h>
#include <attributes/iceberg.h>
#include <attributes/hate.h>

/* util includes */
#include <attrib.h>

void
init_attributes(void)
{
	/* at_iceberg */
	init_iceberg();
	/* at_key */
	init_key();
	/* at_orcification */
	init_orcification();
	/* at_follow */
	init_follow();
	/* at_targetregion */
	init_targetregion();
	/* at_hate */
	init_hate();
	/* at_reduceproduction */
	init_reduceproduction();
}
