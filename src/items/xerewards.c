/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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
#include "xerewards.h"

#include "magic.h"
#include "study.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/curse.h>
#include <kernel/messages.h>
#include <kernel/pool.h>

/* util includes */
#include <util/functions.h>

#include <study.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int
use_skillpotion(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    /* the problem with making this a lua function is that there's no way
     * to get the list of skills for a unit. and with the way skills are
     * currently saved, it doesn't look likely (can't make eressea::list
     * from them)
     */
    int n;
    for (n = 0; n != amount; ++n) {
        skill *sv = u->skills;
        while (sv != u->skills + u->skill_size) {
            /* only one person learns for 3 weeks */
            learn_skill(u, (skill_t)sv->id, STUDYDAYS * 3);
            ++sv;
        }
    }
    ADDMSG(&u->faction->msgs, msg_message("skillpotion_use", "unit", u));
    return amount;
}

int
use_manacrystal(struct unit *u, const struct item_type *itype, int amount,
    struct order *ord)
{
    int i, sp = 0;

    if (!is_mage(u)) {
        cmistake(u, u->thisorder, 295, MSG_EVENT);
        return -1;
    }

    for (i = 0; i != amount; ++i) {
        sp += MAX(25, max_spellpoints(u->region, u) / 2);
        change_spellpoints(u, sp);
    }

    ADDMSG(&u->faction->msgs, msg_message("manacrystal_use", "unit aura", u, sp));

    return amount;
}

void register_xerewards(void)
{
    register_item_use(use_skillpotion, "use_skillpotion");
    register_item_use(use_manacrystal, "use_manacrystal");
}
