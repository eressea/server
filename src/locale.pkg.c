/*
** Lua binding: locale
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
TOLUA_API int tolua_locale_open (lua_State* tolua_S);
LUALIB_API int luaopen_locale (lua_State* tolua_S);

#undef tolua_reg_types
#define tolua_reg_types tolua_reg_types_locale
#include "bind_locale.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: locale_create */
static int tolua_locale_eressea_locale_create00(lua_State* tolua_S)
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
  const char* lang = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  locale_create(lang);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'create'.",&tolua_err);
 return 0;
#endif
}

/* function: locale_set */
static int tolua_locale_eressea_locale_set00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) || 
 !tolua_isstring(tolua_S,2,0,&tolua_err) || 
 !tolua_isstring(tolua_S,3,0,&tolua_err) || 
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  const char* lang = ((const char*)  tolua_tostring(tolua_S,1,0));
  const char* key = ((const char*)  tolua_tostring(tolua_S,2,0));
  const char* str = ((const char*)  tolua_tostring(tolua_S,3,0));
 {
  locale_set(lang,key,str);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set'.",&tolua_err);
 return 0;
#endif
}

/* function: locale_get */
static int tolua_locale_eressea_locale_get00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) || 
 !tolua_isstring(tolua_S,2,0,&tolua_err) || 
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  const char* lang = ((const char*)  tolua_tostring(tolua_S,1,0));
  const char* key = ((const char*)  tolua_tostring(tolua_S,2,0));
 {
  const char* tolua_ret = (const char*)  locale_get(lang,key);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'get'.",&tolua_err);
 return 0;
#endif
}

/* function: locale_direction */
static int tolua_locale_eressea_locale_direction00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) || 
 !tolua_isstring(tolua_S,2,0,&tolua_err) || 
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  const char* lang = ((const char*)  tolua_tostring(tolua_S,1,0));
  const char* str = ((const char*)  tolua_tostring(tolua_S,2,0));
 {
  int tolua_ret = (int)  locale_direction(lang,str);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'direction'.",&tolua_err);
 return 0;
#endif
}

/* Open lib function */
LUALIB_API int luaopen_locale (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_module(tolua_S,"eressea",0);
 tolua_beginmodule(tolua_S,"eressea");
 tolua_module(tolua_S,"locale",0);
 tolua_beginmodule(tolua_S,"locale");
 tolua_function(tolua_S,"create",tolua_locale_eressea_locale_create00);
 tolua_function(tolua_S,"set",tolua_locale_eressea_locale_set00);
 tolua_function(tolua_S,"get",tolua_locale_eressea_locale_get00);
 tolua_function(tolua_S,"direction",tolua_locale_eressea_locale_direction00);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}
/* Open tolua function */
TOLUA_API int tolua_locale_open (lua_State* tolua_S)
{
 lua_pushcfunction(tolua_S, luaopen_locale);
 lua_pushstring(tolua_S, "locale");
 lua_call(tolua_S, 1, 0);
 return 1;
}
