/* vi: set ts=2:
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2006   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *  
 */

/* wenn config.h nicht vor curses included wird, kompiliert es unter windows nicht */
#include <config.h>
#include <eressea.h>

#include "console.h"
#include "gmtool.h"

/* lua includes */
#include "lua/bindings.h"
#include <boost/version.hpp>
#include <lua.hpp>
#include <luabind/luabind.hpp>

static const struct { 
  const char * name;
  int (*func)(lua_State *);
} lualibs[] = {
  {"",                luaopen_base},
  {LUA_TABLIBNAME,    luaopen_table},
  {LUA_IOLIBNAME,     luaopen_io},
  {LUA_STRLIBNAME,    luaopen_string},
  {LUA_MATHLIBNAME,   luaopen_math},
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

static lua_State *
lua_init(void)
{
  lua_State * L = lua_open();

  openlibs(L);

  luabind::open(L);
  bind_objects(L);
  bind_eressea(L);
  bind_spell(L);
  bind_alliance(L);
  bind_region(L);
  bind_item(L);
  bind_faction(L);
  bind_unit(L);
  bind_ship(L);
  bind_building(L);
  bind_gmtool(L);
  lua_readline = curses_readline;
  return L;
}

static void
lua_done(lua_State * L)
{
  lua_close(L);
}

int
main(int argc, char *argv[])
{
  lua_State* luaState = lua_init();
  global.vm_state = luaState;

  int result = gmmain(argc, argv);
  lua_done(luaState);

  return result;
}
