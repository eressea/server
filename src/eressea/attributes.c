/* vi: set ts=2:
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
#include <attributes/attributes.h>

/* attributes includes */
#include <attributes/key.h>
#include <attributes/gm.h>
#include <attributes/orcification.h>
#include <attributes/targetregion.h>
#include <attributes/reduceproduction.h>
#include <attributes/follow.h>
#include <attributes/iceberg.h>
#include <attributes/hate.h>
#include <attributes/overrideroads.h>
#include <attributes/racename.h>
#ifdef AT_OPTION
# include <attributes/option.h>
#endif
#ifdef AT_MOVED
# include <attributes/moved.h>
#endif

/* util includes */
#include <attrib.h>

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
	init_racename();
#ifdef AT_MOVED
	init_moved();
#endif
#ifdef AT_OPTION
	init_option();
#endif
}
