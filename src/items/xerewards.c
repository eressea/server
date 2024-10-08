#include "xerewards.h"

#include <magic.h>
#include <study.h>

/* kernel includes */
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/curse.h>
#include <kernel/messages.h>
#include <kernel/pool.h>
#include <kernel/skills.h>

/* util includes */
#include <util/functions.h>
#include <util/message.h>

#include <stb_ds.h>

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
    if (u->skills) {
        int n;
        for (n = 0; n != amount; ++n) {
            ptrdiff_t s, len = arrlen(u->skills);
            for (s = 0; s != len; ++s) {
                skill* sv = u->skills + s;
                /* only one person learns for 3 weeks */
                change_skill_days(u, (skill_t)sv->id, SKILL_DAYS_PER_WEEK * 3);
            }
        }
    }
    ADDMSG(&u->faction->msgs, msg_message("skillpotion_use", "unit", u));
    return amount;
}

int
use_manacrystal(struct unit *u, const struct item_type *itype, int amount,
    struct order *ord)
{
    int i, sp = 0, msp;

    if (!is_mage(u)) {
        cmistake(u, u->thisorder, 295, MSG_EVENT);
        return -1;
    }

    msp = max_spellpoints(u, u->region) / 2;
    if (msp < 25) msp = 25;
    for (i = 0; i != amount; ++i) {
        sp += msp;
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
