#include <kernel/config.h>
#include "equipment.h"

/* kernel includes */
#include "callbacks.h"
#include "faction.h"
#include "item.h"
#include "race.h"
#include "spell.h"
#include "unit.h"

/* util includes */
#include <selist.h>
#include <critbit.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>

#include <strings.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

bool equip_unit(struct unit *u, const char *eqname)
{
    return equip_unit_mask(u, eqname, EQUIP_ALL);
}

bool equip_unit_mask(struct unit *u, const char *eqname, int mask)
{
    if (callbacks.equip_unit) {
        return callbacks.equip_unit(u, eqname, mask);
    }
    return false;
}
