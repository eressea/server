/* vi: set ts=2:
 *
 * $Id: attributes.c,v 1.5 2001/02/04 08:42:36 enno Exp $
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
#include "key.h"
#include "targetregion.h"
#include "orcification.h"
#include "reduceproduction.h"
#include "follow.h"
#include "iceberg.h"
#include "hate.h"

/* util includes */
#include <attrib.h>

/*
 * library initialization
 */

extern attrib_type at_roads_override;

void
init_attributes(void)
{
	at_register(&at_orcification);
    at_register(&at_roads_override);
	/* at_iceberg */
	init_iceberg();
	/* at_key */
	init_key();
	/* at_follow */
	init_follow();
	/* at_targetregion */
	init_targetregion();
	/* at_orcification */
	init_orcification();
	/* at_hate */
	init_hate();
	/* at_reduceproduction */
	init_reduceproduction();
}
