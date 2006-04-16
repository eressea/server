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
#include "lua/script.h"
#include <boost/version.hpp>
#include <lua.hpp>
#include <luabind/luabind.hpp>

static lua_State *
lua_init(void)
{
  lua_State * L = lua_open();
  luaopen_base(L);
  luaopen_math(L);
  luaopen_string(L);
  luaopen_io(L);
  luaopen_table(L);
#if 0
  luabind::open(L);
  bind_objects(L);
  bind_eressea(L);
  bind_script(L);
  bind_spell(L);
  bind_alliance(L);
  bind_region(L);
  bind_item(L);
  bind_faction(L);
  bind_unit(L);
  bind_ship(L);
  bind_building(L);
  bind_event(L);
  bind_message(L);
#endif
  lua_readline = curses_readline;
  return L;
}

static void
lua_done(lua_State * L)
{
#if 0
  reset_scripts();
#endif
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
