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
    struct selist;
    struct _dictionary_;

    int tolua_sqlite_open(struct lua_State *L);
    int tolua_bindings_open(struct lua_State *L, const struct _dictionary_ *d);
    int tolua_itemlist_next(struct lua_State *L);
    int tolua_orderlist_next(struct lua_State *L);
    int tolua_selist_push(struct lua_State *L, const char *list_type,
        const char *elem_type, struct selist *list);

    int log_lua_error(struct lua_State *L);

    void lua_done(struct lua_State *L);
    struct lua_State *lua_init(const struct _dictionary_ *d);
    int eressea_run(struct lua_State *L, const char *luafile);

#ifdef __cplusplus
}
#endif
