#include "bind_tolua.h"

#include <tolua.h>
#include <lua.h>
#include <luaconf.h>

TOLUA_API int tolua_config_open (lua_State* tolua_S);
LUALIB_API int luaopen_config (lua_State* tolua_S);
TOLUA_API int tolua_eressea_open (lua_State* tolua_S);
LUALIB_API int luaopen_eressea (lua_State* tolua_S);
TOLUA_API int tolua_game_open (lua_State* tolua_S);
LUALIB_API int luaopen_game (lua_State* tolua_S);
TOLUA_API int tolua_log_open (lua_State* tolua_S);
LUALIB_API int luaopen_log (lua_State* tolua_S);
TOLUA_API int tolua_process_open (lua_State* tolua_S);
LUALIB_API int luaopen_process (lua_State* tolua_S);
TOLUA_API int tolua_settings_open (lua_State* tolua_S);
LUALIB_API int luaopen_settings (lua_State* tolua_S);
TOLUA_API int tolua_locale_open (lua_State* tolua_S);
LUALIB_API int luaopen_locale (lua_State* tolua_S);

void tolua_bind_open(lua_State * L)
{
    tolua_eressea_open(L);
    tolua_process_open(L);
    tolua_settings_open(L);
    tolua_game_open(L);
    tolua_config_open(L);
    tolua_locale_open(L);
    tolua_log_open(L);
}
