#include "bind_building.h"
#include "bind_unit.h"

#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/region.h>

#include <util/log.h>
#include <util/language.h>
#include <util/strings.h>

#include <lua.h>
#include <lauxlib.h>
#include <tolua.h>

#include <stdbool.h>
#include <stdlib.h>

int tolua_buildinglist_next(lua_State * L)
{
    building **building_ptr =
        (building **)lua_touserdata(L, lua_upvalueindex(1));
    building *u = *building_ptr;
    if (u != NULL) {
        tolua_pushusertype(L, (void *)u, "building");
        *building_ptr = u->next;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

static int tolua_building_set_working(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    bool flag = !!lua_toboolean(L, 2);
    if (flag) self->flags &= ~BLD_UNMAINTAINED;
    else self->flags |= BLD_UNMAINTAINED;
    return 0;
}

static int tolua_building_get_working(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    bool flag = (self->flags & BLD_UNMAINTAINED) == 0;
    lua_pushboolean(L, flag);
    return 1;
}

static int tolua_building_get_region(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, building_getregion(self), "region");
    return 1;
}

static int tolua_building_set_region(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    building_setregion(self, (region *)tolua_tousertype(L, 2, 0));
    return 0;
}

static int tolua_building_get_info(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, self->display);
    return 1;
}

static int tolua_building_set_info(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    const char *info = tolua_tostring(L, 2, 0);
    free(self->display);
    if (info)
        self->display = str_strdup(info);
    else
        self->display = NULL;
    return 0;
}

static int tolua_building_get_name(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, building_getname(self));
    return 1;
}

static int tolua_building_set_name(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    building_setname(self, tolua_tostring(L, 2, 0));
    return 0;
}

static int tolua_building_get_maxsize(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, self->type->maxsize);
    return 1;
}

static int tolua_building_get_size(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, self->size);
    return 1;
}

static int tolua_building_set_size(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    self->size = (int)tolua_tonumber(L, 2, 0);
    return 0;
}

static int tolua_building_get_units(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    unit **unit_ptr = (unit **)lua_newuserdata(L, sizeof(unit *));
    unit *u = self->region->units;

    while (u && u->building != self)
        u = u->next;
    luaL_getmetatable(L, "unit");
    lua_setmetatable(L, -2);

    *unit_ptr = u;

    lua_pushcclosure(L, tolua_unitlist_nextb, 1);
    return 1;
}

static int tolua_building_get_id(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, self->no);
    return 1;
}

static int tolua_building_get_type(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, self->type->_name);
    return 1;
}

static int tolua_building_get_typename(lua_State * L)
{
    building *b = (building *)tolua_tousertype(L, 1, 0);
    if (b) {
        int size = (int)tolua_tonumber(L, 2, b->size);
        tolua_pushstring(L, buildingtype(b->type, b, size));
        return 1;
    }
    return 0;
}

static int tolua_building_get_owner(lua_State * L)
{
    building *b = (building *)tolua_tousertype(L, 1, 0);
    unit *u = b ? building_owner(b) : NULL;
    tolua_pushusertype(L, u, "unit");
    return 1;
}

static int tolua_building_set_owner(lua_State * L)
{
    building *b = (building *)tolua_tousertype(L, 1, 0);
    unit *u = (unit *)tolua_tousertype(L, 2, 0);
    if (b != u->building) {
        u_set_building(u, b);
    }
    building_set_owner(u);
    return 0;
}

static int tolua_building_create(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    const char *bname = tolua_tostring(L, 2, NULL);
    int size = (int)tolua_tonumber(L, 3, 1);
    if (!r) {
        log_error("building.create expects a region as argument 1");
    } else if (!bname) {
        log_error("building.create expects a name as argument 2");
    }
    else if (size <= 0) {
        log_error("building.create expects a size > 0");
    } else {
        const building_type *btype = bt_find(bname);
        if (btype) {
            building *b = new_building(btype, r, default_locale, size);
            tolua_pushusertype(L, (void *)b, "building");
            return 1;
        }
    }
    return 0;
}

static int tolua_building_tostring(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    lua_pushstring(L, buildingname(self));
    return 1;
}

static int tolua_building_destroy(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    remove_building(&self->region->buildings, self);
    return 0;
}

void tolua_building_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, "building");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_cclass(L, "building", "building", "",
            NULL);
        tolua_beginmodule(L, "building");
        {
            tolua_function(L, "create", tolua_building_create);
            tolua_function(L, "destroy", tolua_building_destroy);
            tolua_function(L, "__tostring", tolua_building_tostring);

            tolua_variable(L, "id", tolua_building_get_id, NULL);
            tolua_variable(L, "owner", tolua_building_get_owner,
                tolua_building_set_owner);
            tolua_variable(L, "type", tolua_building_get_type, NULL);
            tolua_variable(L, "name", tolua_building_get_name,
                tolua_building_set_name);
            tolua_variable(L, "info", tolua_building_get_info,
                tolua_building_set_info);
            tolua_variable(L, "units", tolua_building_get_units, NULL);
            tolua_variable(L, "region", tolua_building_get_region,
                tolua_building_set_region);
            tolua_variable(L, "maxsize", tolua_building_get_maxsize, NULL);
            tolua_variable(L, "size", tolua_building_get_size,
                tolua_building_set_size);
            tolua_function(L, "get_typename", tolua_building_get_typename);
            tolua_variable(L, "working", tolua_building_get_working, tolua_building_set_working);

        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
