/*
** Lua binding: eressea
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
TOLUA_API int tolua_eressea_open (lua_State* tolua_S);
LUALIB_API int luaopen_eressea (lua_State* tolua_S);

#undef tolua_reg_types
#define tolua_reg_types tolua_reg_types_eressea
#include "bind_tolua.h"
#include "bind_eressea.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: eressea_free_game */
static int tolua_eressea_eressea_free_game00(lua_State* tolua_S)
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
  eressea_free_game();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'free_game'.",&tolua_err);
 return 0;
#endif
}

/* function: eressea_read_game */
static int tolua_eressea_eressea_read_game00(lua_State* tolua_S)
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
  int tolua_ret = (int)  eressea_read_game(filename);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'read_game'.",&tolua_err);
 return 0;
#endif
}

/* function: eressea_write_game */
static int tolua_eressea_eressea_write_game00(lua_State* tolua_S)
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
  int tolua_ret = (int)  eressea_write_game(filename);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'write_game'.",&tolua_err);
 return 0;
#endif
}

/* function: eressea_read_orders */
static int tolua_eressea_eressea_read_orders00(lua_State* tolua_S)
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
  int tolua_ret = (int)  eressea_read_orders(filename);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'read_orders'.",&tolua_err);
 return 0;
#endif
}

/* function: eressea_export_json */
static int tolua_eressea_eressea_export00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) || 
 !tolua_isnumber(tolua_S,2,0,&tolua_err) || 
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  const char* filename = ((const char*)  tolua_tostring(tolua_S,1,0));
  unsigned int flags = ((unsigned int)  tolua_tonumber(tolua_S,2,0));
 {
  int tolua_ret = (int)  eressea_export_json(filename,flags);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'export'.",&tolua_err);
 return 0;
#endif
}

/* function: eressea_import_json */
static int tolua_eressea_eressea_import00(lua_State* tolua_S)
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
  int tolua_ret = (int)  eressea_import_json(filename);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'import'.",&tolua_err);
 return 0;
#endif
}

/* Open lib function */
LUALIB_API int luaopen_eressea (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_module(tolua_S,"eressea",0);
 tolua_beginmodule(tolua_S,"eressea");
 tolua_function(tolua_S,"free_game",tolua_eressea_eressea_free_game00);
 tolua_function(tolua_S,"read_game",tolua_eressea_eressea_read_game00);
 tolua_function(tolua_S,"write_game",tolua_eressea_eressea_write_game00);
 tolua_function(tolua_S,"read_orders",tolua_eressea_eressea_read_orders00);
 tolua_function(tolua_S,"export",tolua_eressea_eressea_export00);
 tolua_function(tolua_S,"import",tolua_eressea_eressea_import00);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}
/* Open tolua function */
TOLUA_API int tolua_eressea_open (lua_State* tolua_S)
{
 lua_pushcfunction(tolua_S, luaopen_eressea);
 lua_pushstring(tolua_S, "eressea");
 lua_call(tolua_S, 1, 0);
 return 1;
}
