#include "bind_ship.h"
#include "bind_unit.h"

#include "move.h"

#include "kernel/attrib.h"
#include "kernel/curse.h"
#include "kernel/direction.h"
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/build.h>

#include <util/language.h>
#include <util/log.h>

#include <tolua.h>
#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>

int tolua_shiplist_next(lua_State * L)
{
    ship **ship_ptr = (ship **)lua_touserdata(L, lua_upvalueindex(1));
    ship *u = *ship_ptr;
    if (u != NULL) {
        tolua_pushusertype(L, (void *)u, "ship");
        *ship_ptr = u->next;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

static int tolua_ship_get_number(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, sh->number);
    return 1;
}

static int tolua_ship_set_number(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    int n = (int)tolua_tonumber(L, 2, 0);
    scale_ship(sh, n);
    return 0;
}

static int tolua_ship_get_id(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, sh->no);
    return 1;
}

static int tolua_ship_get_name(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, ship_getname(sh));
    return 1;
}

static int tolua_ship_get_size(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, sh->size);
    return 1;
}

static int tolua_ship_get_display(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, sh->display);
    return 1;
}

static int tolua_ship_get_region(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    if (sh) {
        tolua_pushusertype(L, sh->region, "region");
        return 1;
    }
    return 0;
}

static int tolua_ship_set_region(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    region *r = (region *)tolua_tousertype(L, 2, NULL);
    if (sh) {
        move_ship(sh, sh->region, r, NULL);
    }
    return 0;
}

static int tolua_ship_get_owner(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    if (sh) {
        tolua_pushusertype(L, ship_owner(sh), "unit");
        return 1;
    }
    return 0;
}

static int tolua_ship_set_owner(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    unit *u = (unit *)tolua_tousertype(L, 2, NULL);
    if (sh && u->ship == sh) {
        ship_set_owner(u);
    }
    return 0;
}

static int tolua_ship_set_name(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    ship_setname(sh, tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_ship_set_size(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    sh->size = (int)lua_tointeger(L, 2);
    return 0;
}

static int tolua_ship_set_display(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    ship_setinfo(sh, tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_ship_get_units(lua_State * L)
{
#ifndef TOLUA_RELEASE
tolua_Error tolua_err;
if (
!tolua_isusertype(L,1,"ship",0,&tolua_err) || 
!tolua_isuserdata(L,2,0,&tolua_err) || 
!tolua_isnoobj(L,3,&tolua_err)
) goto tolua_lerror;
else
#endif
 {
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    unit **unit_ptr = (unit **)lua_newuserdata(L, sizeof(unit *));
    unit *u = sh->region->units;

    while (u && u->ship != sh)
        u = u->next;
    luaL_getmetatable(L, "unit");
    lua_setmetatable(L, -2);

    *unit_ptr = u;

    lua_pushcclosure(L, tolua_unitlist_nexts, 1);
    return 1;
 }
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(L, "#ferror in function 'export'.", &tolua_err);
 return 0;
#endif
}

static int tolua_ship_create(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    const char *sname = tolua_tostring(L, 2, NULL);
    if (sname) {
        const ship_type *stype = st_find(sname);
        if (stype) {
            ship *sh = new_ship(stype, r, default_locale);
            sh->size = stype->construction ? stype->construction->maxsize : 1;
            tolua_pushusertype(L, (void *)sh, "ship");
            return 1;
        }
        else {
            log_error("Unknown ship type '%s'\n", sname);
        }
    }
    return 0;
}

static int
tolua_ship_tostring(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, shipname(sh));
    return 1;
}

static int tolua_ship_get_flags(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, sh->flags);
    return 1;
}

static int tolua_ship_set_flags(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    sh->flags = (int)lua_tointeger(L, 2);
    return 0;
}

static int tolua_ship_set_coast(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    if (lua_isnil(L, 2)) {
        sh->coast = NODIRECTION;
    }
    else if (lua_isnumber(L, 2)) {
        sh->coast = (direction_t)lua_tointeger(L, 2);
    }
    return 0;
}

static int tolua_ship_get_coast(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    if (sh->coast) {
        lua_pushinteger(L, sh->coast);
        return 1;
    }
    return 0;
}

static int tolua_ship_get_type(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, sh->type->_name);
    return 1;
}

static int tolua_ship_get_damage(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, sh->damage);
    return 1;
}

static int tolua_ship_set_damage(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    sh->damage = (int)lua_tointeger(L, 2);
    return 0;
}

static int tolua_ship_get_curse(lua_State *L) {
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    if (sh->attribs) {
        curse * c = get_curse(sh->attribs, ct_find(name));
        if (c) {
            lua_pushnumber(L, curse_geteffect(c));
            return 1;
        }
    }
    return 0;
}

static int tolua_ship_has_attrib(lua_State *L) {
    ship *sh = (ship *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    attrib * a = a_find(sh->attribs, at_find(name));
    lua_pushboolean(L, a != NULL);
    return 1;
}

void tolua_ship_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, "ship");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_cclass(L, "ship", "ship", "", NULL);
        tolua_beginmodule(L, "ship");
        {
            tolua_function(L, "__tostring", tolua_ship_tostring);
            tolua_variable(L, "id", tolua_ship_get_id, NULL);
            tolua_variable(L, "number", tolua_ship_get_number, tolua_ship_set_number);
            tolua_variable(L, "name", tolua_ship_get_name,
                tolua_ship_set_name);
            tolua_variable(L, "size", tolua_ship_get_size,
                tolua_ship_set_size);
            tolua_variable(L, "info", tolua_ship_get_display,
                tolua_ship_set_display);
            tolua_variable(L, "units", tolua_ship_get_units, NULL);
            tolua_variable(L, "flags", &tolua_ship_get_flags,
                tolua_ship_set_flags);
            tolua_variable(L, "region", tolua_ship_get_region,
                tolua_ship_set_region);
            tolua_variable(L, "coast", tolua_ship_get_coast,
                tolua_ship_set_coast);
            tolua_variable(L, "type", tolua_ship_get_type, 0);
            tolua_variable(L, "damage", tolua_ship_get_damage,
                tolua_ship_set_damage);
            tolua_variable(L, "owner", tolua_ship_get_owner,
                tolua_ship_set_owner);

            tolua_function(L, "get_curse", &tolua_ship_get_curse);
            tolua_function(L, "has_attrib", &tolua_ship_has_attrib);

            tolua_function(L, "create", tolua_ship_create);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
