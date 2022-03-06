#pragma once
#ifndef H_BIND_REGION
#define H_BIND_REGION

  struct lua_State;
  void tolua_region_open(struct lua_State *L);
  int tolua_regionlist_next(struct lua_State *L);

#endif
