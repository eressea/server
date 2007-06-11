/* vi: set ts=2:
 * Eressea PB(E)M host   Christian Schlittchen (corwin@amber.kn-bremen.de)
 * (C) 1998-2004         Katja Zedel (katze@felidae.kn-bremen.de)
 *                       Enno Rehling (enno@eressea-pbem.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 **/

#ifdef __cplusplus
extern "C" {
#endif
#include <lua.hpp>

  extern int lua_console(lua_State * L);
  extern int lua_do(lua_State * L);
  extern int (*lua_readline)(lua_State *l, const char *prompt);

#ifdef __cplusplus
}
#endif

