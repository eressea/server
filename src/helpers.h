/* 
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
  void register_tolua_helpers(void);
  int tolua_toid(struct lua_State *L, int idx, int def);

#ifdef __cplusplus
}
#endif
