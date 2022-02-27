#pragma once
#ifndef H_BIND_SHIP
#define H_BIND_SHIP

struct lua_State;

int tolua_shiplist_next(struct lua_State *L);
void tolua_ship_open(struct lua_State *L);

#endif
