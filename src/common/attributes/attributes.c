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
#include <eressea.h>
#include "attributes.h"

/* attributes includes */
#include "key.h"
#include "gm.h"
#include "targetregion.h"
#include "orcification.h"
#include "reduceproduction.h"
#include "follow.h"
#include "iceberg.h"
#include "hate.h"
#include "overrideroads.h"
#include "otherfaction.h"
#include "racename.h"
#ifdef USE_UGROUPS
#  include "ugroup.h"
#endif
#ifdef AT_OPTION
# include "option.h"
#endif
#ifdef AT_MOVED
# include "moved.h"
#endif

/* util includes */
#include <attrib.h>

/*
 * library initialization
 */

void
init_attributes(void)
{
	at_register(&at_overrideroads);

	init_iceberg();
	init_key();
	init_gm();
	init_follow();
	init_targetregion();
	init_orcification();
	init_hate();
	init_reduceproduction();
	init_otherfaction();
	init_racename();
#ifdef AT_MOVED
	init_moved();
#endif
#ifdef AT_OPTION
	init_option();
#endif
#ifdef USE_UGROUPS
	init_ugroup();
#endif
}
