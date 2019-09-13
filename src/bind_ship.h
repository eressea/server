#ifdef __cplusplus
extern "C" {
#endif

  struct lua_State;
  int tolua_shiplist_next(struct lua_State *L);
  void tolua_ship_open(struct lua_State *L);

#ifdef __cplusplus
}
#endif
