#include <platform.h>
#include "settings.h"
#include "eressea.h"

#include <kernel/config.h>
#include <util/console.h>
#include <util/log.h>

/* lua includes */
#ifdef BINDINGS_TOLUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <bindings/bindings.h>
#include <bindings/helpers.h>
#include <bindings/bind_attrib.h>
#include <bindings/bind_building.h>
#include <bindings/bind_faction.h>
#include <bindings/bind_gmtool.h>
#include <bindings/bind_hashtable.h>
#include <bindings/bind_message.h>
#include <bindings/bind_region.h>
#include <bindings/bind_ship.h>
#include <bindings/bind_storage.h>
#include <bindings/bind_unit.h>
#endif // BINDINGS_TOLUA



static const struct {
  const char * name;
  int (*func)(lua_State *);
} lualibs[] = {
  {"",                luaopen_base},
  {LUA_TABLIBNAME,    luaopen_table},
  {LUA_IOLIBNAME,     luaopen_io},
  {LUA_STRLIBNAME,    luaopen_string},
  {LUA_MATHLIBNAME,   luaopen_math},
  {LUA_LOADLIBNAME,   luaopen_package},
  {LUA_DBLIBNAME,     luaopen_debug},
#if LUA_VERSION_NUM>=501
  {LUA_OSLIBNAME,     luaopen_os},
#endif
  { NULL, NULL }
};

static void
openlibs(lua_State * L)
{
  int i;
  for (i=0;lualibs[i].func;++i) {
    lua_pushcfunction(L, lualibs[i].func);
    lua_pushstring(L, lualibs[i].name);
    lua_call(L, 1, 0);
  }
}

static void
lua_done(lua_State * L)
{
  lua_close(L);
}

static lua_State *
lua_init(void)
{
  lua_State * L = lua_open();

  openlibs(L);
#ifdef BINDINGS_TOLUA
  register_tolua_helpers();
  tolua_eressea_open(L);
  tolua_sqlite_open(L);
  tolua_unit_open(L);
  tolua_building_open(L);
  tolua_ship_open(L);
  tolua_region_open(L);
  tolua_faction_open(L);
  tolua_attrib_open(L);
  tolua_unit_open(L);
  tolua_message_open(L);
  tolua_hashtable_open(L);
  tolua_gmtool_open(L);
  tolua_storage_open(L);
#endif
  return L;
}

static void
game_done(void)
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
}

static void
game_init(void)
{
  init_triggers();
  init_xmas();

  reports_init();
  report_init();
  creport_init();
  xmlreport_init();

  debug_language("locales.log");
  register_races();
  register_names();
  register_resources();
  register_buildings();
  register_itemfunctions();
#ifdef TODO
  register_curses();
  register_spells();
  register_gcspells();
#endif
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
  register_gmcmd();

}

int eressea_init(void)
{
  global.vm_state = lua_init();
  kernel_init();
  game_init();

  return 0;
}

void eressea_done(void)
{
  game_done();
  kernel_done();
  lua_done((lua_State *)global.vm_state);
  log_close();
}

static int
log_lua_error(lua_State * L)
{
  static int s_abort_on_errors = 0;
  const char* error = lua_tostring(L, -1);

  log_error(("A LUA error occurred: %s\n", error));
  lua_pop(L, 1);

  if (s_abort_on_errors) {
    abort();
  }
  return 1;
}

int eressea_run(const char * luafile, const char * entry_point)
{
  lua_State * L = (lua_State *)global.vm_state;
  /* run the main script */
  if (luafile) {
    lua_getglobal(L, "dofile");
    lua_pushstring(L, luafile);
    if (lua_pcall(L, 1, 0, 0) != 0) {
      log_lua_error(L);
    }
  }
  if (entry_point) {
    lua_getglobal(L, entry_point);
    if (lua_pcall(L, 0, 1, 0) != 0) {
      log_lua_error(L);
    }
  } else {
    lua_console(L);
  }
  return 0;
}
