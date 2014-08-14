/*
** Lua binding: config
*/

#include "tolua.h"

#ifndef __cplusplus
#include <stdlib.h>
#endif
#ifdef __cplusplus
 extern "C" int tolua_bnd_takeownership (lua_State* L); // from tolua_map.c
#else
 int tolua_bnd_takeownership (lua_State* L); /* from tolua_map.c */
#endif
#include <string.h>

/* Exported function */
TOLUA_API int tolua_config_open (lua_State* tolua_S);
LUALIB_API int luaopen_config (lua_State* tolua_S);

#undef tolua_reg_types
#define tolua_reg_types tolua_reg_types_config
#include "bind_config.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: config_reset */
static int tolua_config_eressea_config_reset00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  config_reset();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'reset'.",&tolua_err);
 return 0;
#endif
}

/* function: config_read */
static int tolua_config_eressea_config_read00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) || 
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  const char* filename = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  int tolua_ret = (int)  config_read(filename);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'read'.",&tolua_err);
 return 0;
#endif
}

/* function: config_parse */
static int tolua_config_eressea_config_parse00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) || 
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  const char* json = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  int tolua_ret = (int)  config_parse(json);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'parse'.",&tolua_err);
 return 0;
#endif
}

/* Open lib function */
LUALIB_API int luaopen_config (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_module(tolua_S,"eressea",0);
 tolua_beginmodule(tolua_S,"eressea");
 tolua_module(tolua_S,"config",0);
 tolua_beginmodule(tolua_S,"config");
 tolua_function(tolua_S,"reset",tolua_config_eressea_config_reset00);
 tolua_function(tolua_S,"read",tolua_config_eressea_config_read00);
 tolua_function(tolua_S,"parse",tolua_config_eressea_config_parse00);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}
/* Open tolua function */
TOLUA_API int tolua_config_open (lua_State* tolua_S)
{
 lua_pushcfunction(tolua_S, luaopen_config);
 lua_pushstring(tolua_S, "config");
 lua_call(tolua_S, 1, 0);
 return 1;
}
