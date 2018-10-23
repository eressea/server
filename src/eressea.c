#include <platform.h>
#include "eressea.h"

#include "kernel/building.h"
#include "kernel/calendar.h"
#include "kernel/database.h"
#include "kernel/config.h"
#include "kernel/curse.h"
#include "kernel/equipment.h"
#include "kernel/faction.h"
#include "kernel/item.h"

#include "util/functions.h"
#include "util/language.h"
#include "util/log.h"
#include "util/message.h"

#include "modules/gmcmd.h"
#include "modules/xmas.h"
#include "modules/museum.h"
#include "triggers/triggers.h"
#include "items/xerewards.h"
#include "items/weapons.h"
#include "attributes/attributes.h"
#include "races/races.h"

#include "items.h"
#include "creport.h"
#include "report.h"
#include "names.h"
#include "orderdb.h"
#include "reports.h"
#include "spells.h"
#include "vortex.h"
#include "wormhole.h"

#include <errno.h>
#include <stdlib.h>

/* manually free() everything at exit? */
#undef CLEANUP_CODE

void game_done(void)
{
    log_dead_factions();

#ifdef CLEANUP_CODE
    free_gamedata();
    creport_cleanup();
    report_cleanup();
    calendar_cleanup();
    free_functions();
    free_config();
    free_special_directions();
    free_locales();
#endif
    kernel_done();
    swapdb_close();
}

void game_init(void)
{
    swapdb_open();
    errno = 0;
    kernel_init();
    register_triggers();
    register_xmas();

    register_nr();
    register_cr();

    register_races();
    register_spells();
    register_names();
    register_resources();
    register_itemfunctions();
    register_museum();
    wormholes_register();

    register_weapons();
    register_xerewards();
    register_attributes();
    register_gmcmd();

}
