/*
** Lua binding: process
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
TOLUA_API int tolua_process_open (lua_State* tolua_S);
LUALIB_API int luaopen_process (lua_State* tolua_S);

#undef tolua_reg_types
#define tolua_reg_types tolua_reg_types_process
#include "bind_process.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* function: process_update_long_order */
static int tolua_process_eressea_process_update_long_order00(lua_State* tolua_S)
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
  process_update_long_order();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'update_long_order'.",&tolua_err);
 return 0;
#endif
}

/* function: process_markets */
static int tolua_process_eressea_process_markets00(lua_State* tolua_S)
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
  process_markets();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'markets'.",&tolua_err);
 return 0;
#endif
}

/* function: process_produce */
static int tolua_process_eressea_process_produce00(lua_State* tolua_S)
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
  process_produce();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'produce'.",&tolua_err);
 return 0;
#endif
}

/* function: process_make_temp */
static int tolua_process_eressea_process_make_temp00(lua_State* tolua_S)
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
  process_make_temp();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'make_temp'.",&tolua_err);
 return 0;
#endif
}

/* function: process_settings */
static int tolua_process_eressea_process_settings00(lua_State* tolua_S)
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
  process_settings();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'settings'.",&tolua_err);
 return 0;
#endif
}

/* function: process_ally */
static int tolua_process_eressea_process_set_allies00(lua_State* tolua_S)
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
  process_ally();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_allies'.",&tolua_err);
 return 0;
#endif
}

/* function: process_prefix */
static int tolua_process_eressea_process_set_prefix00(lua_State* tolua_S)
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
  process_prefix();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_prefix'.",&tolua_err);
 return 0;
#endif
}

/* function: process_setstealth */
static int tolua_process_eressea_process_set_stealth00(lua_State* tolua_S)
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
  process_setstealth();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_stealth'.",&tolua_err);
 return 0;
#endif
}

/* function: process_status */
static int tolua_process_eressea_process_set_status00(lua_State* tolua_S)
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
  process_status();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_status'.",&tolua_err);
 return 0;
#endif
}

/* function: process_name */
static int tolua_process_eressea_process_set_name00(lua_State* tolua_S)
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
  process_name();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_name'.",&tolua_err);
 return 0;
#endif
}

/* function: process_group */
static int tolua_process_eressea_process_set_group00(lua_State* tolua_S)
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
  process_group();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_group'.",&tolua_err);
 return 0;
#endif
}

/* function: process_origin */
static int tolua_process_eressea_process_set_origin00(lua_State* tolua_S)
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
  process_origin();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_origin'.",&tolua_err);
 return 0;
#endif
}

/* function: process_quit */
static int tolua_process_eressea_process_quit00(lua_State* tolua_S)
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
  process_quit();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'quit'.",&tolua_err);
 return 0;
#endif
}

/* function: process_study */
static int tolua_process_eressea_process_study00(lua_State* tolua_S)
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
  process_study();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'study'.",&tolua_err);
 return 0;
#endif
}

/* function: process_movement */
static int tolua_process_eressea_process_movement00(lua_State* tolua_S)
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
  process_movement();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'movement'.",&tolua_err);
 return 0;
#endif
}

/* function: process_use */
static int tolua_process_eressea_process_use00(lua_State* tolua_S)
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
  process_use();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'use'.",&tolua_err);
 return 0;
#endif
}

/* function: process_battle */
static int tolua_process_eressea_process_battle00(lua_State* tolua_S)
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
  process_battle();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'battle'.",&tolua_err);
 return 0;
#endif
}

/* function: process_siege */
static int tolua_process_eressea_process_siege00(lua_State* tolua_S)
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
  process_siege();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'siege'.",&tolua_err);
 return 0;
#endif
}

/* function: process_leave */
static int tolua_process_eressea_process_leave00(lua_State* tolua_S)
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
  process_leave();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'leave'.",&tolua_err);
 return 0;
#endif
}

/* function: process_maintenance */
static int tolua_process_eressea_process_maintenance00(lua_State* tolua_S)
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
  process_maintenance();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'maintenance'.",&tolua_err);
 return 0;
#endif
}

/* function: process_promote */
static int tolua_process_eressea_process_promote00(lua_State* tolua_S)
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
  process_promote();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'promote'.",&tolua_err);
 return 0;
#endif
}

/* function: process_restack */
static int tolua_process_eressea_process_restack00(lua_State* tolua_S)
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
  process_restack();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'restack'.",&tolua_err);
 return 0;
#endif
}

/* function: process_setspells */
static int tolua_process_eressea_process_set_spells00(lua_State* tolua_S)
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
  process_setspells();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_spells'.",&tolua_err);
 return 0;
#endif
}

/* function: process_sethelp */
static int tolua_process_eressea_process_set_help00(lua_State* tolua_S)
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
  process_sethelp();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_help'.",&tolua_err);
 return 0;
#endif
}

/* function: process_contact */
static int tolua_process_eressea_process_contact00(lua_State* tolua_S)
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
  process_contact();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'contact'.",&tolua_err);
 return 0;
#endif
}

/* function: process_enter */
static int tolua_process_eressea_process_enter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) || 
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  int message = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  process_enter(message);
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'enter'.",&tolua_err);
 return 0;
#endif
}

/* function: process_magic */
static int tolua_process_eressea_process_magic00(lua_State* tolua_S)
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
  process_magic();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'magic'.",&tolua_err);
 return 0;
#endif
}

/* function: process_give_control */
static int tolua_process_eressea_process_give_control00(lua_State* tolua_S)
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
  process_give_control();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'give_control'.",&tolua_err);
 return 0;
#endif
}

/* function: process_regeneration */
static int tolua_process_eressea_process_regeneration00(lua_State* tolua_S)
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
  process_regeneration();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'regeneration'.",&tolua_err);
 return 0;
#endif
}

/* function: process_guard_on */
static int tolua_process_eressea_process_guard_on00(lua_State* tolua_S)
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
  process_guard_on();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'guard_on'.",&tolua_err);
 return 0;
#endif
}

/* function: process_guard_off */
static int tolua_process_eressea_process_guard_off00(lua_State* tolua_S)
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
  process_guard_off();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'guard_off'.",&tolua_err);
 return 0;
#endif
}

/* function: process_explain */
static int tolua_process_eressea_process_explain00(lua_State* tolua_S)
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
  process_explain();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'explain'.",&tolua_err);
 return 0;
#endif
}

/* function: process_messages */
static int tolua_process_eressea_process_messages00(lua_State* tolua_S)
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
  process_messages();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'messages'.",&tolua_err);
 return 0;
#endif
}

/* function: process_reserve */
static int tolua_process_eressea_process_reserve00(lua_State* tolua_S)
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
  process_reserve();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'reserve'.",&tolua_err);
 return 0;
#endif
}

/* function: process_claim */
static int tolua_process_eressea_process_claim00(lua_State* tolua_S)
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
  process_claim();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'claim'.",&tolua_err);
 return 0;
#endif
}

/* function: process_follow */
static int tolua_process_eressea_process_follow00(lua_State* tolua_S)
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
  process_follow();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'follow'.",&tolua_err);
 return 0;
#endif
}

/* function: process_alliance */
static int tolua_process_eressea_process_alliance00(lua_State* tolua_S)
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
  process_alliance();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'alliance'.",&tolua_err);
 return 0;
#endif
}

/* function: process_idle */
static int tolua_process_eressea_process_idle00(lua_State* tolua_S)
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
  process_idle();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'idle'.",&tolua_err);
 return 0;
#endif
}

/* function: process_set_default */
static int tolua_process_eressea_process_set_default00(lua_State* tolua_S)
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
  process_set_default();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'set_default'.",&tolua_err);
 return 0;
#endif
}

/* Open lib function */
LUALIB_API int luaopen_process (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_module(tolua_S,"eressea",0);
 tolua_beginmodule(tolua_S,"eressea");
 tolua_module(tolua_S,"process",0);
 tolua_beginmodule(tolua_S,"process");
 tolua_function(tolua_S,"update_long_order",tolua_process_eressea_process_update_long_order00);
 tolua_function(tolua_S,"markets",tolua_process_eressea_process_markets00);
 tolua_function(tolua_S,"produce",tolua_process_eressea_process_produce00);
 tolua_function(tolua_S,"make_temp",tolua_process_eressea_process_make_temp00);
 tolua_function(tolua_S,"settings",tolua_process_eressea_process_settings00);
 tolua_function(tolua_S,"set_allies",tolua_process_eressea_process_set_allies00);
 tolua_function(tolua_S,"set_prefix",tolua_process_eressea_process_set_prefix00);
 tolua_function(tolua_S,"set_stealth",tolua_process_eressea_process_set_stealth00);
 tolua_function(tolua_S,"set_status",tolua_process_eressea_process_set_status00);
 tolua_function(tolua_S,"set_name",tolua_process_eressea_process_set_name00);
 tolua_function(tolua_S,"set_group",tolua_process_eressea_process_set_group00);
 tolua_function(tolua_S,"set_origin",tolua_process_eressea_process_set_origin00);
 tolua_function(tolua_S,"quit",tolua_process_eressea_process_quit00);
 tolua_function(tolua_S,"study",tolua_process_eressea_process_study00);
 tolua_function(tolua_S,"movement",tolua_process_eressea_process_movement00);
 tolua_function(tolua_S,"use",tolua_process_eressea_process_use00);
 tolua_function(tolua_S,"battle",tolua_process_eressea_process_battle00);
 tolua_function(tolua_S,"siege",tolua_process_eressea_process_siege00);
 tolua_function(tolua_S,"leave",tolua_process_eressea_process_leave00);
 tolua_function(tolua_S,"maintenance",tolua_process_eressea_process_maintenance00);
 tolua_function(tolua_S,"promote",tolua_process_eressea_process_promote00);
 tolua_function(tolua_S,"restack",tolua_process_eressea_process_restack00);
 tolua_function(tolua_S,"set_spells",tolua_process_eressea_process_set_spells00);
 tolua_function(tolua_S,"set_help",tolua_process_eressea_process_set_help00);
 tolua_function(tolua_S,"contact",tolua_process_eressea_process_contact00);
 tolua_function(tolua_S,"enter",tolua_process_eressea_process_enter00);
 tolua_function(tolua_S,"magic",tolua_process_eressea_process_magic00);
 tolua_function(tolua_S,"give_control",tolua_process_eressea_process_give_control00);
 tolua_function(tolua_S,"regeneration",tolua_process_eressea_process_regeneration00);
 tolua_function(tolua_S,"guard_on",tolua_process_eressea_process_guard_on00);
 tolua_function(tolua_S,"guard_off",tolua_process_eressea_process_guard_off00);
 tolua_function(tolua_S,"explain",tolua_process_eressea_process_explain00);
 tolua_function(tolua_S,"messages",tolua_process_eressea_process_messages00);
 tolua_function(tolua_S,"reserve",tolua_process_eressea_process_reserve00);
 tolua_function(tolua_S,"claim",tolua_process_eressea_process_claim00);
 tolua_function(tolua_S,"follow",tolua_process_eressea_process_follow00);
 tolua_function(tolua_S,"alliance",tolua_process_eressea_process_alliance00);
 tolua_function(tolua_S,"idle",tolua_process_eressea_process_idle00);
 tolua_function(tolua_S,"set_default",tolua_process_eressea_process_set_default00);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}
/* Open tolua function */
TOLUA_API int tolua_process_open (lua_State* tolua_S)
{
 lua_pushcfunction(tolua_S, luaopen_process);
 lua_pushstring(tolua_S, "process");
 lua_call(tolua_S, 1, 0);
 return 1;
}
