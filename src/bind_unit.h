#ifdef __cplusplus
extern "C" {
#endif

  struct lua_State;
  int tolua_unitlist_nextb(struct lua_State *L);
  int tolua_unitlist_nexts(struct lua_State *L);
  int tolua_unitlist_nextf(struct lua_State *L);
  int tolua_unitlist_next(struct lua_State *L);
  void tolua_unit_open(struct lua_State *L);

#ifdef __cplusplus
}
#endif
