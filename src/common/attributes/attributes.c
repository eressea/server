/* vi: set ts=2:
 *
 * $Id: attributes.c,v 1.3 2001/01/27 19:30:07 enno Exp $
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
#include "reduceproduction.h"
#include "follow.h"
#include "iceberg.h"
#include "hate.h"

/* util includes */
#include <attrib.h>

/*
 * simple attributes that do not yet have their own file 
 */

attrib_type at_orcification = {
	"orcification", NULL, NULL, NULL, a_writedefault, a_readdefault, ATF_UNIQUE
};

/*
 * library initialization
 */


void
init_attributes(void)
{
	at_register(&at_orcification);

	/* at_iceberg */
	init_iceberg();
	/* at_key */
	init_key();
	/* at_follow */
	init_follow();
	/* at_targetregion */
	init_targetregion();
	/* at_hate */
	init_hate();
	/* at_reduceproduction */
	init_reduceproduction();
}
