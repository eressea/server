/*
Copyright (c) 1998-2015,
Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include "guard.h"
#include "laws.h"
#include "monster.h"

#include <kernel/ally.h>
#include <kernel/save.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/building.h>

#include <assert.h>

guard_t can_start_guarding(const unit * u)
{
    if (u->status >= ST_FLEE || fval(u, UFL_FLEEING))
        return E_GUARD_FLEEING;
    /* Monster der Monsterpartei dürfen immer bewachen */
    if (is_monsters(u->faction) || fval(u_race(u), RCF_UNARMEDGUARD))
        return E_GUARD_OK;
    if (!armedmen(u, true))
        return E_GUARD_UNARMED;
    if (IsImmune(u->faction))
        return E_GUARD_NEWBIE;
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
                    setguard(u, GUARD_NONE);
                }
            }
        }
    }
}

unsigned int guard_flags(const unit * u)
{
    // TODO: this should be a property of the race, like race.guard_flags
    static int rc_cache;
    static const race *rc_elf, *rc_ent, *rc_ironkeeper;
    unsigned int flags =
        GUARD_CREWS | GUARD_LANDING | GUARD_TRAVELTHRU | GUARD_TAX;
    // TODO: configuration, not define
#if GUARD_DISABLES_PRODUCTION == 1
    flags |= GUARD_PRODUCE;
#endif
#if GUARD_DISABLES_RECRUIT == 1
    flags |= GUARD_RECRUIT;
#endif
    return flags;
}

void setguard(unit * u, unsigned int flags)
{
    bool enabled = (flags!=GUARD_NONE);
    if (!enabled) {
        assert(!fval(u, UFL_MOVED));
        assert(u->status < ST_FLEE);
        freset(u, UFL_GUARD);
    } else {
        fset(u, UFL_GUARD);
        fset(u->region, RF_GUARDED);
    }
}

unsigned int getguard(const unit * u)
{
    assert(fval(u, UFL_GUARD) || (u->building && u == building_owner(u->building))
        || !"you're doing it wrong! check is_guard first");
    return guard_flags(u);
}

void guard(unit * u, unsigned int mask)
{
    unsigned int flags = guard_flags(u);
    setguard(u, flags & mask);
}

static bool is_guardian_u(const unit * guard, unit * u, unsigned int mask)
{
    if (guard->faction == u->faction)
        return false;
    if (is_guard(guard, mask) == 0)
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

static bool is_guardian_r(const unit * guard)
{
    if (guard->number == 0)
        return false;
    if (besieged(guard))
        return false;

    /* if region_owners exist then they may be guardians: */
    if (guard->building && rule_region_owners() && guard == building_owner(guard->building)) {
        faction *owner = region_get_owner(guard->region);
        if (owner == guard->faction) {
            building *bowner = largestbuilding(guard->region, &cmp_taxes, false);
            if (bowner == guard->building) {
                return true;
            }
        }
    }

    if ((guard->flags & UFL_GUARD) == 0)
        return false;
    return fval(u_race(guard), RCF_UNARMEDGUARD) || is_monsters(guard->faction) || (armedmen(guard, true) > 0);
}

bool is_guard(const struct unit * u, unsigned int mask)
{
    return is_guardian_r(u) && (getguard(u) & mask) != 0;
}

unit *is_guarded(region * r, unit * u, unsigned int mask)
{
    unit *u2;
    int noguards = 1;

    if (!fval(r, RF_GUARDED)) {
        return NULL;
    }

    /* at this point, u2 is the last unit we tested to
    * be a guard (and failed), or NULL
    * i is the position of the first free slot in the cache */

    for (u2 = r->units; u2; u2 = u2->next) {
        if (is_guardian_r(u2)) {
            noguards = 0;
            if (is_guardian_u(u2, u, mask)) {
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
