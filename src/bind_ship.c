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
#include "bind_ship.h"
#include "bind_unit.h"

#include "move.h"

#include <kernel/curse.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/build.h>

#include <util/attrib.h>
#include <util/language.h>
#include <util/log.h>
#include <util/strings.h>

#include <tolua.h>
#include <string.h>
#include <stdlib.h>

int tolua_shiplist_next(lua_State * L)
{
    ship **ship_ptr = (ship **)lua_touserdata(L, lua_upvalueindex(1));
    ship *u = *ship_ptr;
    if (u != NULL) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "ship");
        *ship_ptr = u->next;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

static int tolua_ship_get_id(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->no);
    return 1;
}

static int tolua_ship_get_name(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, ship_getname(self));
    return 1;
}

static int tolua_ship_get_size(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->size);
    return 1;
}

static int tolua_ship_get_display(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, self->display);
    return 1;
}

static int tolua_ship_get_region(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    if (self) {
        tolua_pushusertype(L, self->region, TOLUA_CAST "region");
        return 1;
    }
    return 0;
}

static int tolua_ship_set_region(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    region *r = (region *)tolua_tousertype(L, 2, NULL);
    if (self) {
        move_ship(self, self->region, r, NULL);
    }
    return 0;
}

static int tolua_ship_set_name(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    ship_setname(self, tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_ship_set_size(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    self->size = lua_tointeger(L, 2);
    return 0;
}

static int tolua_ship_set_display(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    free(self->display);
    self->display = str_strdup(tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_ship_get_units(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    unit **unit_ptr = (unit **)lua_newuserdata(L, sizeof(unit *));
    unit *u = self->region->units;

    while (u && u->ship != self)
        u = u->next;
    luaL_getmetatable(L, TOLUA_CAST "unit");
    lua_setmetatable(L, -2);

    *unit_ptr = u;

    lua_pushcclosure(L, tolua_unitlist_nexts, 1);
    return 1;
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
            tolua_pushusertype(L, (void *)sh, TOLUA_CAST "ship");
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
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, shipname(self));
    return 1;
}

static int tolua_ship_get_flags(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->flags);
    return 1;
}

static int tolua_ship_set_flags(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    self->flags = (int)lua_tointeger(L, 2);
    return 0;
}

static int tolua_ship_set_coast(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    if (lua_isnil(L, 2)) {
        self->coast = NODIRECTION;
    }
    else if (lua_isnumber(L, 2)) {
        self->coast = (direction_t)lua_tointeger(L, 2);
    }
    return 0;
}

static int tolua_ship_get_coast(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    if (self->coast) {
        lua_pushinteger(L, self->coast);
        return 1;
    }
    return 0;
}

static int tolua_ship_get_type(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, self->type->_name);
    return 1;
}

static int tolua_ship_get_damage(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->damage);
    return 1;
}

static int tolua_ship_set_damage(lua_State * L)
{
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    self->damage = (int)lua_tointeger(L, 2);
    return 0;
}

static int tolua_ship_get_curse(lua_State *L) {
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    if (self->attribs) {
        curse * c = get_curse(self->attribs, ct_find(name));
        if (c) {
            lua_pushnumber(L, curse_geteffect(c));
            return 1;
        }
    }
    return 0;
}

static int tolua_ship_has_attrib(lua_State *L) {
    ship *self = (ship *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    attrib * a = a_find(self->attribs, at_find(name));
    lua_pushboolean(L, a != NULL);
    return 1;
}

void tolua_ship_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "ship");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_cclass(L, TOLUA_CAST "ship", TOLUA_CAST "ship", TOLUA_CAST "", NULL);
        tolua_beginmodule(L, TOLUA_CAST "ship");
        {
            tolua_function(L, TOLUA_CAST "__tostring", tolua_ship_tostring);
            tolua_variable(L, TOLUA_CAST "id", tolua_ship_get_id, NULL);
            tolua_variable(L, TOLUA_CAST "name", tolua_ship_get_name,
                tolua_ship_set_name);
            tolua_variable(L, TOLUA_CAST "size", tolua_ship_get_size,
                tolua_ship_set_size);
            tolua_variable(L, TOLUA_CAST "info", tolua_ship_get_display,
                tolua_ship_set_display);
            tolua_variable(L, TOLUA_CAST "units", tolua_ship_get_units, NULL);
            tolua_variable(L, TOLUA_CAST "flags", &tolua_ship_get_flags,
                tolua_ship_set_flags);
            tolua_variable(L, TOLUA_CAST "region", tolua_ship_get_region,
                tolua_ship_set_region);
            tolua_variable(L, TOLUA_CAST "coast", tolua_ship_get_coast,
                tolua_ship_set_coast);
            tolua_variable(L, TOLUA_CAST "type", tolua_ship_get_type, 0);
            tolua_variable(L, TOLUA_CAST "damage", tolua_ship_get_damage,
                tolua_ship_set_damage);

            tolua_function(L, TOLUA_CAST "get_curse", &tolua_ship_get_curse);
            tolua_function(L, TOLUA_CAST "has_attrib", &tolua_ship_has_attrib);

            tolua_function(L, TOLUA_CAST "create", tolua_ship_create);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
