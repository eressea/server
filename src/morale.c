#include "morale.h"

#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/curse.h"
#include "kernel/region.h"
#include "kernel/faction.h"
#include "kernel/race.h"
#include "kernel/building.h"

#include <util/rand.h>

#include <spells/regioncurse.h>

#include <assert.h>

static double popularity(void)
{
    return 1.0 / (MORALE_AVERAGE - MORALE_COOLDOWN); /* 10 turns average */
}

void morale_update(region *r) {
    int now = turn + 1;
    int morale = region_get_morale(r);
    assert(r->land);

    if (r->land->ownership && r->land->ownership->owner) {
        int stability = now - r->land->ownership->morale_turn;
        int maxmorale = MORALE_DEFAULT;
        building *b = largestbuilding(r, cmp_taxes, false);
        if (b) {
            int bsize = buildingeffsize(b, false);
            assert(b->type->taxes>0);
            maxmorale = (bsize + 1) * MORALE_TAX_FACTOR / b->type->taxes;
        }
        if (morale < maxmorale) {
            if (stability > MORALE_COOLDOWN && r->land->ownership->owner
                && morale < MORALE_MAX) {
                double ch = popularity();
                if (is_cursed(r->attribs, &ct_generous)) {
                    ch *= 1.2;            /* 20% improvement */
                }
                if (stability >= MORALE_AVERAGE * 2 || chance(ch)) {
                    region_set_morale(r, morale + 1, now);
                }
            }
        }
        else if (morale > maxmorale) {
            region_set_morale(r, morale - 1, now);
        }
    }
    else if (morale > MORALE_DEFAULT) {
        region_set_morale(r, morale - 1, now);
    }
}

void morale_change(region *r, int value) {
    int morale = region_get_morale(r);
    if (morale > 0) {
        morale = morale - value;
        if (morale < 0) morale = 0;
        region_set_morale(r, morale, turn);
    }
}
