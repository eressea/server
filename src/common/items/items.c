/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "items.h"

#include "lmsreward.h"
#include "demonseye.h"
#include "weapons.h"
#include "xerewards.h"
#include "seed.h"
#include "birthday_firework.h"

void
register_items(void)
{
	register_weapons();
	register_demonseye();
	init_lmsreward();
	register_xerewards();
	init_seed();
	init_mallornseed();
	register_birthday_firework();
	register_lebkuchenherz();
}
