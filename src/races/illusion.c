/*
 * Eressea PB(E)M host Copyright (C) 1998-2015
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

/* kernel includes */
#include <kernel/race.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/messages.h>

/* libc includes */
#include <stdlib.h>
#include <assert.h>

#define ILLUSIONMAX  6

void age_illusion(unit * u)
{
    if (u->faction->race != get_race(RC_ILLUSION)) {
        if (u->age == ILLUSIONMAX) {
            ADDMSG(&u->faction->msgs, msg_message("warnillusiondissolve", "unit", u));
        }
        else if (u->age > ILLUSIONMAX) {
            set_number(u, 0);
            ADDMSG(&u->faction->msgs, msg_message("illusiondissolve", "unit", u));
        }
    }
}
