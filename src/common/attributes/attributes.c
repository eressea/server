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

#include <platform.h>
#include <kernel/config.h>
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
#ifdef WDW_PYRAMID
# include "alliance.h"
#endif

/* kernel includes */
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/building.h>

/* util includes */
#include <util/attrib.h>

attrib_type at_unitdissolve = {
  "unitdissolve", NULL, NULL, NULL, a_writechars, a_readchars
};

void
register_attributes(void)
{
  at_register(&at_object);
  at_register(&at_unitdissolve);
  at_register(&at_overrideroads);
  at_register(&at_raceprefix);
  at_register(&at_iceberg);
  at_register(&at_key);
  at_register(&at_gm);
  at_register(&at_follow);
  at_register(&at_targetregion);
  at_register(&at_orcification);
  at_register(&at_hate);
  at_register(&at_reduceproduction);
  at_register(&at_otherfaction);
  at_register(&at_racename);
  at_register(&at_movement);
  at_register(&at_moved);

#ifdef WDW_PYRAMID
  at_register(&at_alliance);
#endif /* WDW_PYRAMID */
}
