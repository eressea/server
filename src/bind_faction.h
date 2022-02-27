#pragma once
#ifndef H_BIND_FACTION
#define H_BIND_FACTION

struct lua_State;

int tolua_factionlist_next(struct lua_State *L);
void tolua_faction_open(struct lua_State *L);

#endif
