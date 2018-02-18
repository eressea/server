#include <platform.h>
#include "eressea.h"

#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/curse.h"
#include "kernel/building.h"
#include "kernel/equipment.h"
#include "kernel/item.h"
#include "kernel/database.h"

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

#include "chaos.h"
#include "items.h"
#include "creport.h"
#include "report.h"
#include "names.h"
#include "reports.h"
#include "spells.h"
#include "vortex.h"
#include "wormhole.h"
#include "xmlreader.h"

#include <errno.h>
#include <stdlib.h>

void game_done(void)
{
#undef CLEANUP_CODE
#ifdef CLEANUP_CODE
    /* Diese Routine enfernt allen allokierten Speicher wieder. Das ist nur
     * zum Debugging interessant, wenn man Leak Detection hat, und nach
     * nicht freigegebenem Speicher sucht, der nicht bis zum Ende benoetigt
     * wird (temporaere Hilsstrukturen) */

    free_game();

    creport_cleanup();
#ifdef REPORT_FORMAT_NR
    report_cleanup();
#endif
#endif
    calendar_cleanup();
    free_functions();
    free_config();
    free_special_directions();
    free_locales();
    kernel_done();
    dblib_close();
}

void game_init(void)
{
    dblib_open();
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
    register_xmlreader();
    register_attributes();
    register_gmcmd();

    chaos_register();
}
