/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "itemtypes.h"

#include "demonseye.h"
#include "xerewards.h"
#include "artrewards.h"
#include "weapons.h"
#include "racespoils.h"
#if GROWING_TREES
# include "seed.h"
#endif

void
register_itemtypes(void)
{
	register_weapons();
	register_demonseye();
	register_xerewards();
#if GROWING_TREES
	register_seed();
	register_mallornseed();
#endif
	register_racespoils();
  register_artrewards();
}

void
init_itemtypes(void)
{
	init_weapons();
}
