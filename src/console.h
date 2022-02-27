#pragma once
#ifndef H_LUA_CONSOLE
#define H_LUA_CONSOLE

#include <stddef.h>

struct lua_State;

int lua_console(struct lua_State *L);
int lua_do(struct lua_State *L);

typedef int (*readline_fun) (struct lua_State *, char *, size_t, const char *);
void set_readline(readline_fun foo);

#endif
