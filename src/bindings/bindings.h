/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#ifdef __cplusplus
extern "C" {
#endif

  struct lua_State;
  int tolua_sqlite_open(struct lua_State * L);
  int tolua_eressea_open(struct lua_State* L);
  int tolua_spelllist_next(struct lua_State *L);
  int tolua_itemlist_next(struct lua_State *L);
  int tolua_orderlist_next(struct lua_State *L);

  int log_lua_error(struct lua_State * L);
#ifdef __cplusplus
}
#endif
