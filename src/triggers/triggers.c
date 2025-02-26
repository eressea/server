#include "triggers.h"

/* triggers includes */
#include <triggers/changerace.h>
#include <triggers/createcurse.h>
#include <triggers/createunit.h>
#include <triggers/gate.h>
#include <triggers/giveitem.h>
#include <triggers/killunit.h>
#include <triggers/shock.h>
#include <triggers/timeout.h>
#include <triggers/clonedied.h>

/* util includes */
#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <kernel/event.h>

#include <storage.h>

/* libc includes */
#include <stdio.h>

/* tt_changefaction was once used by sp_charming song,
 * and now only needs to exist for loading from old files
 */

static int changefaction_read(trigger *t, gamedata *data)
{
    READ_INT(data->store, NULL);
    READ_INT(data->store, NULL);
    return AT_READ_FAIL;
}

trigger_type tt_changefaction = {
    .name = "changefaction",
    .read = changefaction_read
};

void register_triggers(void)
{
    tt_register(&tt_changefaction);
    tt_register(&tt_changerace);
    tt_register(&tt_createcurse);
    tt_register(&tt_createunit);
    tt_register(&tt_gate);
    tt_register(&tt_giveitem);
    tt_register(&tt_killunit);
    tt_register(&tt_shock);
    tt_register(&tt_timeout);
    tt_register(&tt_clonedied);
}
