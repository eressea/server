#ifndef LUA_BINDINGS_H
#define LUA_BINDINGS_H

extern bool is_function(struct lua_State * L, const char * fname);

extern void bind_region(struct lua_State * L);
extern void bind_unit(struct lua_State * L);
extern void bind_ship(struct lua_State * L);
extern void bind_building(struct lua_State * L);
extern void bind_faction(struct lua_State * L);
extern void bind_alliance(struct lua_State * L);
extern void bind_eressea(struct lua_State * L);
extern void bind_spell(struct lua_State * L) ;
extern void bind_item(struct lua_State * L);
extern void bind_event(struct lua_State * L);
extern void bind_message(struct lua_State * L);
extern void bind_objects(struct lua_State * L);

/* test routines */
extern void bind_test(struct lua_State * L);

/* server only */
extern void bind_script(struct lua_State * L);
extern void bind_gamecode(struct lua_State * L);

/* gmtool only */
extern void bind_gmtool(lua_State * L);

#endif
