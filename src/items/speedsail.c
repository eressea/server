#include "speedsail.h"

/* kernel includes */
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <attributes/movement.h>

/* util includes */
#include <kernel/attrib.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>

/* libc includes */
#include <assert.h>

static int
use_speedsail(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    struct plane *p = rplane(u->region);
    UNUSED_ARG(amount);
    UNUSED_ARG(itype);
    if (p != NULL) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "use_realworld_only", ""));
    }
    else {
        if (u->ship) {
            if (set_speedup(&u->ship->attribs, 50, 50)) {
                ADDMSG(&u->faction->msgs, msg_message("use_speedsail", "unit", u));
            }
            else {
                cmistake(u, ord, 211, MSG_EVENT);
                /* Segel verbrauchen */
                i_change(&u->items, itype, -1);
                return 0;
            }
        }
        else {
            cmistake(u, ord, 144, MSG_EVENT);
        }
    }
    return EUNUSABLE;
}

void register_speedsail(void)
{
    register_item_use(use_speedsail, "use_speedsail");
}
