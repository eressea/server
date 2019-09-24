#include <platform.h>

/* triggers includes */
#include <triggers/changefaction.h>
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
#include <kernel/event.h>

/* libc includes */
#include <stdio.h>

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
