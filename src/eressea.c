#include <platform.h>
#include "settings.h"
#include "eressea.h"

#include <kernel/config.h>
#include <util/log.h>

#if MUSEUM_MODULE
#include <modules/museum.h>
#endif
#if ARENA_MODULE
#include <modules/arena.h>
#endif
#include <triggers/triggers.h>
#include <util/language.h>
#include <util/functions.h>
#include <kernel/xmlreader.h>
#include <kernel/item.h>
#include <kernel/building.h>
#include <modules/gmcmd.h>
#include <modules/xmas.h>
#include <items/itemtypes.h>
#include <attributes/attributes.h>
#include <races/races.h>
#include "chaos.h"
#include "report.h"
#include "items.h"
#include "creport.h"
#include "jsreport.h"
#include "names.h"
#include "wormhole.h"
#include "spells.h"

void game_done(void)
{
#ifdef CLEANUP_CODE
    /* Diese Routine enfernt allen allokierten Speicher wieder. Das ist nur
     * zum Debugging interessant, wenn man Leak Detection hat, und nach
     * nicht freigegebenem Speicher sucht, der nicht bis zum Ende benötigt
     * wird (temporäre Hilsstrukturen) */

    free_game();

    creport_cleanup();
#ifdef REPORT_FORMAT_NR
    report_cleanup();
#endif
    calendar_cleanup();
#endif
    free_functions();
    kernel_done();
}

void game_init(void)
{
    kernel_init();
    register_triggers();
    register_xmas();

    register_nr();
    register_cr();
    register_jsreport();

    register_races();
    register_spells();
    register_names();
    register_resources();
    register_buildings();
    register_itemfunctions();
#if MUSEUM_MODULE
    register_museum();
#endif
#if ARENA_MODULE
    register_arena();
#endif
    wormholes_register();

    register_itemtypes();
#ifdef USE_LIBXML2
    register_xmlreader();
#endif
    register_attributes();
    register_gmcmd();

    chaos_register();
}
