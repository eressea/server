#ifndef LUA_EXPORT_H
#define LUA_EXPORT_H

struct lua_State;

extern void bind_region(struct lua_State * L);
extern void bind_unit(struct lua_State * L);
extern void bind_ship(struct lua_State * L);
extern void bind_building(struct lua_State * L);
extern void bind_faction(struct lua_State * L);

#endif
