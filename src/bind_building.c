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

#include <platform.h>
#include "bind_building.h"
#include "bind_unit.h"
#include "bind_dict.h"

#include <kernel/config.h>
#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/region.h>

#include <util/language.h>

#include <tolua.h>
#include <stdlib.h>
#include <string.h>

int tolua_buildinglist_next(lua_State * L)
{
    building **building_ptr =
        (building **)lua_touserdata(L, lua_upvalueindex(1));
    building *u = *building_ptr;
    if (u != NULL) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "building");
        *building_ptr = u->next;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

static int tolua_building_get_objects(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, (void *)&self->attribs, USERTYPE_DICT);
    return 1;
}

static int tolua_building_set_working(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    bool flag = !!lua_toboolean(L, 2);
    if (flag) self->flags |= BLD_WORKING;
    else self->flags &= ~BLD_WORKING;
    return 1;
}

static int tolua_building_get_working(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    bool flag = (self->flags&BLD_WORKING) != 0;
    lua_pushboolean(L, flag);
    return 1;
}

static int tolua_building_get_region(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, building_getregion(self), TOLUA_CAST "region");
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
        self->display = _strdup(info);
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

static int tolua_building_get_size(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushnumber(L, self->size);
    return 1;
}

static int tolua_building_set_size(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    self->size = (int)tolua_tonumber(L, 2, 0);
    return 0;
}

static int tolua_building_get_damage(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    tolua_pushnumber(L, self->damage);
    return 1;
}

static int tolua_building_set_damage(lua_State * L)
{
    building *self = (building *)tolua_tousertype(L, 1, 0);
    self->damage = (int)tolua_tonumber(L, 2, 0);
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
    tolua_pushnumber(L, (lua_Number)self->no);
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
    tolua_pushusertype(L, u, TOLUA_CAST "unit");
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
    region *r = (region *)tolua_tousertype(L, 1, 0);
    const char *bname = tolua_tostring(L, 2, 0);
    if (bname) {
        const building_type *btype = bt_find(bname);
        if (btype) {
            building *b = new_building(btype, r, default_locale);
            tolua_pushusertype(L, (void *)b, TOLUA_CAST "building");
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
    tolua_usertype(L, TOLUA_CAST "building");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_cclass(L, TOLUA_CAST "building", TOLUA_CAST "building", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "building");
        {
            tolua_function(L, TOLUA_CAST "create", tolua_building_create);
            tolua_function(L, TOLUA_CAST "destroy", tolua_building_destroy);
            tolua_function(L, TOLUA_CAST "__tostring", tolua_building_tostring);

            tolua_variable(L, TOLUA_CAST "id", tolua_building_get_id, NULL);
            tolua_variable(L, TOLUA_CAST "owner", tolua_building_get_owner,
                tolua_building_set_owner);
            tolua_variable(L, TOLUA_CAST "type", tolua_building_get_type, NULL);
            tolua_variable(L, TOLUA_CAST "name", tolua_building_get_name,
                tolua_building_set_name);
            tolua_variable(L, TOLUA_CAST "info", tolua_building_get_info,
                tolua_building_set_info);
            tolua_variable(L, TOLUA_CAST "units", tolua_building_get_units, NULL);
            tolua_variable(L, TOLUA_CAST "region", tolua_building_get_region,
                tolua_building_set_region);
            tolua_variable(L, TOLUA_CAST "size", tolua_building_get_size,
                tolua_building_set_size);
            tolua_variable(L, TOLUA_CAST "damage", tolua_building_get_damage,
                tolua_building_set_damage);
            tolua_function(L, TOLUA_CAST "get_typename", tolua_building_get_typename);
            tolua_variable(L, TOLUA_CAST "objects", tolua_building_get_objects, 0);
            tolua_variable(L, TOLUA_CAST "working", tolua_building_get_working, tolua_building_set_working);

        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
