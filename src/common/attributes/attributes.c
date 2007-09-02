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
#include <eressea.h>
#include "attributes.h"

/* attributes includes */
#include "follow.h"
#include "gm.h"
#include "hate.h"
#include "iceberg.h"
#include "key.h"
#include "moved.h"
#include "movement.h"
#include "object.h"
#include "orcification.h"
#include "otherfaction.h"
#include "overrideroads.h"
#include "racename.h"
#include "raceprefix.h"
#include "reduceproduction.h"
#include "targetregion.h"
#include "variable.h"
#ifdef AT_OPTION
# include "option.h"
#endif
#ifdef WDW_PYRAMID
# include "alliance.h"
#endif

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/building.h>

/* util includes */
#include <util/attrib.h>

/*
 * library initialization
 */

void
init_attributes(void)
{
  at_register(&at_object);
	at_register(&at_overrideroads);
	at_register(&at_raceprefix);

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
	init_movement();

	init_moved();
#ifdef AT_OPTION
	init_option();
#endif
	init_variable();
#ifdef WDW_PYRAMID
  init_alliance();
#endif
}
