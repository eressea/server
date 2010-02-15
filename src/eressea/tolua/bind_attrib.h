/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2010   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#ifdef __cplusplus
extern "C" {
#endif

  struct attrib;
  void tolua_attrib_open(struct lua_State *L);
  struct attrib * tolua_get_lua_ext(struct attrib * alist);
  int tolua_attriblist_next(struct lua_State *L);

#ifdef __cplusplus
}
#endif
