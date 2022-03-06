#pragma once
#ifndef BIND_ERESSEA_BUILDING
#define BIND_ERESSEA_BUILDING

struct lua_State;

int tolua_buildinglist_next(struct lua_State *L);
void tolua_building_open(struct lua_State *L);

#endif
