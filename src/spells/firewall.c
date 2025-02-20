#include "firewall.h"

#include "spells.h"
#include "borders.h"

#include <magic.h>

#include <kernel/connection.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <math.h>

int sp_firewall(castorder *co)
{
    connection *b;
    wall_data *fd;
    region *r = co_get_region(co);
    unit *caster = co_get_caster(co);
    int cast_level = co->level;
    double force = co->force;
    spellparameter *param = co->a_params;
    int dir;
    region *r2;

    dir = (int)get_direction(param->data.xs, caster->faction->locale);
    if (dir >= 0) {
        r2 = rconnect(r, dir);
    }
    else {
        report_spell_failure(caster, co->order);
        return 0;
    }

    if (!r2 || r2 == r) {
        report_spell_failure(caster, co->order);
        return 0;
    }

    b = get_borders(r, r2);
    while (b != NULL) {
        if (b->type == &bt_firewall)
            break;
        b = b->next;
    }
    if (b == NULL) {
        b = create_border(&bt_firewall, r, r2);
        fd = (wall_data *)b->data.v;
        fd->force = (int)(force / 2 + 0.5);
        fd->mage = caster;
        fd->active = false;
        fd->countdown = cast_level + 1;
    }
    else {
        fd = (wall_data *)b->data.v;
        fd->force = (int)fmax(fd->force, force / 2 + 0.5);
        if (fd->countdown < cast_level + 1)
            fd->countdown = cast_level + 1;
    }

    /* melden, 1x pro Partei */
    {
        message *seen = msg_message("firewall_effect", "mage region", caster, r);
        message *unseen = msg_message("firewall_effect", "mage region", (unit *)NULL, r);
        report_spell_effect(r, caster, seen, unseen);
        msg_release(seen);
        msg_release(unseen);
    }

    return cast_level;
}

