/* vi: set ts=2:
 * Eressea PB(E)M host   Christian Schlittchen (corwin@amber.kn-bremen.de)
 * (C) 1998-2004         Katja Zedel (katze@felidae.kn-bremen.de)
 *                       Enno Rehling (enno@eressea.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 **/

#ifndef H_LUA_CONSOLE
#define H_LUA_CONSOLE
#ifdef __cplusplus
extern "C" {
#endif

  struct lua_State;

  extern int lua_console(struct lua_State * L);
  extern int lua_do(struct lua_State * L);

  typedef int (*readline)(struct lua_State *, char *, size_t, const char *);
  extern void set_readline(readline foo);

#ifdef __cplusplus
}
#endif

#endif
