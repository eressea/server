#include <kernel/config.h>
#include "guard.h"

#include "battle.h"
#include "contact.h"
#include "laws.h"
#include "monsters.h"

#include <kernel/ally.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <assert.h>

guard_t can_start_guarding(const unit * u)
{
    if (u->region->terrain->flags & SEA_REGION) {
        return E_GUARD_TERRAIN;
    }
    if (u->status >= ST_FLEE || fval(u, UFL_FLEEING))
        return E_GUARD_FLEEING;
    /* Monster der Monsterpartei duerfen immer bewachen */
    if (IS_MONSTERS(u->faction) || fval(u_race(u), RCF_UNARMEDGUARD))
        return E_GUARD_OK;
    if (!armedmen(u, true))
        return E_GUARD_UNARMED;
    if (IsImmune(u->faction, faction_age(u->faction) + 1)) {
        /* can be attacked next week, may guard now */
        return E_GUARD_NEWBIE;
    }
    return E_GUARD_OK;
}

void update_guards(void)
{
    const region *r;

    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            if (fval(u, UFL_GUARD)) {
                if (can_start_guarding(u) != E_GUARD_OK) {
                    setguard(u, false);
                }
            }
        }
    }
}

void guard(unit * u)
{
    setguard(u, true);
}

static bool is_guardian_u(const unit * guard, unit * u)
{
    if (guard->faction == u->faction)
        return false;
    if (is_guard(guard) == 0)
        return false;
    if (alliedunit(guard, u->faction, HELP_GUARD))
        return false;
    if (ucontact(guard, u))
        return false;
    if (!cansee(guard->faction, u->region, u, 0))
        return false;
    if (!(u_race(guard)->flags & RCF_FLY) && u_race(u)->flags & RCF_FLY)
        return false;

    return true;
}

static bool is_guardian_r(const unit * u)
{
    if (u->number == 0 || IS_PAUSED(u->faction))
        return false;

    /* if region_owners exist then they may be guardians: */
    if (u->building && rule_region_owners() && u == building_owner(u->building)) {
        faction *owner = region_get_owner(u->region);
        if (owner == u->faction) {
            building *bowner = largestbuilding(u->region, cmp_taxes, false);
            if (bowner == u->building) {
                return true;
            }
        }
    }

    if ((u->flags & UFL_GUARD) == 0)
        return false;
    return fval(u_race(u), RCF_UNARMEDGUARD) || IS_MONSTERS(u->faction) || (armedmen(u, true) > 0);
}

bool is_guard(const struct unit * u)
{
    return is_guardian_r(u);
}

unit *is_guarded(region * r, unit * u)
{
    unit *u2;
    bool noguards = true;

    if (!fval(r, RF_GUARDED)) {
        return NULL;
    }

    /* at this point, u2 is the last unit we tested to
    * be a guard (and failed), or NULL
    * i is the position of the first free slot in the cache */

    for (u2 = r->units; u2; u2 = u2->next) {
        if (is_guardian_r(u2)) {
            noguards = false;
            if (is_guardian_u(u2, u)) {
                /* u2 is our guard. stop processing (we might have to go further next time) */
                return u2;
            }
        }
    }

    if (noguards) {
        /* you are mistaken, sir. there are no guards in these lands */
        freset(r, RF_GUARDED);
    }
    return NULL;
}
