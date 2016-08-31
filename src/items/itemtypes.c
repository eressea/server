/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#include <platform.h>
#include <kernel/config.h>
#include "itemtypes.h"

#include "xerewards.h"
#include "artrewards.h"
#include "weapons.h"
#include "seed.h"

void register_itemtypes(void)
{
    /* registering misc. functions */
    register_weapons();
    register_xerewards();
    register_artrewards();
}

void init_itemtypes(void)
{
    init_seed();
    init_mallornseed();
}
