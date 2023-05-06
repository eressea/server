/*
** Lua binding: log
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
TOLUA_API int tolua_log_open (lua_State* tolua_S);
LUALIB_API int luaopen_log (lua_State* tolua_S);

#undef tolua_reg_types
#define tolua_reg_types tolua_reg_types_log
#include <util/log.h>

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: log_error */
static int tolua_log_eressea_log_error00(lua_State* tolua_S)
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
  const char* message = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  log_error(message);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'error'.",&tolua_err);
 return 0;
#endif
}

/* function: log_debug */
static int tolua_log_eressea_log_debug00(lua_State* tolua_S)
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
  const char* message = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  log_debug(message);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'debug'.",&tolua_err);
 return 0;
#endif
}

/* function: log_warning */
static int tolua_log_eressea_log_warning00(lua_State* tolua_S)
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
  const char* message = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  log_warning(message);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'warning'.",&tolua_err);
 return 0;
#endif
}

/* function: log_info */
static int tolua_log_eressea_log_info00(lua_State* tolua_S)
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
  const char* message = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  log_info(message);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'info'.",&tolua_err);
 return 0;
#endif
}

/* Open lib function */
LUALIB_API int luaopen_log (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_module(tolua_S,"eressea",0);
 tolua_beginmodule(tolua_S,"eressea");
 tolua_module(tolua_S,"log",0);
 tolua_beginmodule(tolua_S,"log");
 tolua_function(tolua_S,"error",tolua_log_eressea_log_error00);
 tolua_function(tolua_S,"debug",tolua_log_eressea_log_debug00);
 tolua_function(tolua_S,"warning",tolua_log_eressea_log_warning00);
 tolua_function(tolua_S,"info",tolua_log_eressea_log_info00);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}
/* Open tolua function */
TOLUA_API int tolua_log_open (lua_State* tolua_S)
{
 lua_pushcfunction(tolua_S, luaopen_log);
 lua_pushstring(tolua_S, "log");
 lua_call(tolua_S, 1, 0);
 return 1;
}
