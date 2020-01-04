#ifndef H_LUA_CONSOLE
#define H_LUA_CONSOLE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

  struct lua_State;

  extern int lua_console(struct lua_State *L);
  extern int lua_do(struct lua_State *L);

  typedef int (*readline_fun) (struct lua_State *, char *, size_t, const char *);
  extern void set_readline(readline_fun foo);

#ifdef __cplusplus
}
#endif
#endif
