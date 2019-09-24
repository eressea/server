#ifdef __cplusplus
extern "C" {
#endif

  struct lua_State;
  int tolua_factionlist_next(struct lua_State *L);
  void tolua_faction_open(struct lua_State *L);

#ifdef __cplusplus
}
#endif
