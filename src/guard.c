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

#include <kernel/save.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/building.h>
#include <util/attrib.h>

#include <assert.h>

attrib_type at_guard = {
    "guard",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writeint,
    a_readint,
    NULL,
    ATF_UNIQUE
};

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
                else {
                    attrib *a = a_find(u->attribs, &at_guard);
                    if (a && a->data.i == (int)guard_flags(u)) {
                        /* this is really rather not necessary */
                        a_remove(&u->attribs, a);
                    }
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
    const race *rc = u_race(u);
    unsigned int flags =
        GUARD_CREWS | GUARD_LANDING | GUARD_TRAVELTHRU | GUARD_TAX;
    // TODO: configuration, not define
#if GUARD_DISABLES_PRODUCTION == 1
    flags |= GUARD_PRODUCE;
#endif
#if GUARD_DISABLES_RECRUIT == 1
    flags |= GUARD_RECRUIT;
#endif
    if (rc_changed(&rc_cache)) {
        rc_elf = get_race(RC_ELF);
        rc_ent = get_race(RC_TREEMAN);
        rc_ironkeeper = get_race(RC_IRONKEEPER);
    }
    if (rc == rc_elf) {
        if (u->faction->race == u_race(u)) {
            flags |= GUARD_TREES;
        }
    }
    else if (rc == rc_ent) {
        flags |= GUARD_TREES;
    }
    else if (rc == rc_ironkeeper) {
        flags = GUARD_MINING;
    }
    return flags;
}

void setguard(unit * u, unsigned int flags)
{
    /* setzt die guard-flags der Einheit */
    attrib *a = NULL;
    assert(flags == 0 || !fval(u, UFL_MOVED));
    assert(flags == 0 || u->status < ST_FLEE);
    if (fval(u, UFL_GUARD)) {
        a = a_find(u->attribs, &at_guard);
    }
    if (flags == GUARD_NONE) {
        freset(u, UFL_GUARD);
        if (a)
            a_remove(&u->attribs, a);
        return;
    }
    fset(u, UFL_GUARD);
    fset(u->region, RF_GUARDED);
    if (flags == guard_flags(u)) {
        if (a)
            a_remove(&u->attribs, a);
    }
    else {
        if (!a)
            a = a_add(&u->attribs, a_new(&at_guard));
        a->data.i = (int)flags;
    }
}

unsigned int getguard(const unit * u)
{
    attrib *a;

    assert(fval(u, UFL_GUARD) || (u->building && u == building_owner(u->building))
        || !"you're doing it wrong! check is_guard first");
    a = a_find(u->attribs, &at_guard);
    if (a) {
        return (unsigned int)a->data.i;
    }
    return guard_flags(u);
}

void guard(unit * u, unsigned int mask)
{
    unsigned int flags = guard_flags(u);
    setguard(u, flags & mask);
}
