#ifndef LUA_BINDINGS_H
#define LUA_BINDINGS_H

struct lua_State;

extern void bind_region(struct lua_State * L);
extern void bind_unit(struct lua_State * L);
extern void bind_ship(struct lua_State * L);
extern void bind_building(struct lua_State * L);
extern void bind_faction(struct lua_State * L);
extern void bind_alliance(struct lua_State * L);
extern void bind_eressea(struct lua_State * L);
extern void bind_spell(lua_State * L) ;
extern void bind_item(struct lua_State * L);
extern void bind_event(lua_State * L);
#endif
