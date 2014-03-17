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
#include <kernel/xmlreader.h>
#include <kernel/reports.h>
#include <kernel/item.h>
#include <kernel/names.h>
#include <kernel/reports.h>
#include <kernel/building.h>
#include <modules/wormhole.h>
#include <modules/xmas.h>
#include <items/itemtypes.h>
#include <attributes/attributes.h>
#include "archetype.h"
#include "report.h"
#include "items.h"
#include "creport.h"
#include "xmlreport.h"

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
  kernel_done();
}

void game_init(void)
{
  kernel_init();
  register_triggers();
  register_xmas();

  register_reports();
  register_nr();
  register_cr();
  register_xr();

  register_names();
  register_resources();
  register_buildings();
  register_itemfunctions();
#if DUNGEON_MODULE
  register_dungeon();
#endif
#if MUSEUM_MODULE
  register_museum();
#endif
#if ARENA_MODULE
  register_arena();
#endif
  register_wormholes();

  register_itemtypes();
  register_xmlreader();
  register_archetypes();
  enable_xml_gamecode();

  register_attributes();

}
