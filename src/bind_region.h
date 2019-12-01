#ifdef __cplusplus
extern "C" {
#endif

  struct lua_State;
  void tolua_region_open(struct lua_State *L);
  int tolua_regionlist_next(struct lua_State *L);

#ifdef __cplusplus
}
#endif
