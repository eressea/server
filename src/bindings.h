#pragma once
#ifndef H_BINDINGS
#define H_BINDINGS

struct lua_State;
struct _dictionary_;
struct selist;

int tolua_toid(struct lua_State *L, int idx, int def);
int tolua_bindings_open(struct lua_State *L, const struct _dictionary_ *d);
int tolua_itemlist_next(struct lua_State *L);
int tolua_selist_push(struct lua_State *L, const char *list_type,
    const char *elem_type, struct selist *list);

void bind_monsters(struct lua_State *L);

int log_lua_error(struct lua_State *L);

void lua_done(struct lua_State *L);
struct lua_State *lua_init(const struct _dictionary_ *d);
int eressea_run(struct lua_State *L, const char *luafile);

#endif
