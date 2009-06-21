/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <config.h>
#include "bind_ship.h"
#include "bind_unit.h"

#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/build.h>

#include <util/language.h>

#include <lua.h>
#include <tolua.h>

int tolua_shiplist_next(lua_State *L)
{
  ship** ship_ptr = (ship **)lua_touserdata(L, lua_upvalueindex(1));
  ship * u = *ship_ptr;
  if (u != NULL) {
    tolua_pushusertype(L, (void*)u, "ship");
    *ship_ptr = u->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int
tolua_ship_get_id(lua_State* L)
{
  ship * self = (ship *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->no);
  return 1;
}

static int tolua_ship_get_name(lua_State* L)
{
  ship* self = (ship*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, ship_getname(self));
  return 1;
}

static int tolua_ship_get_region(lua_State* L)
{
  ship* self = (ship*) tolua_tousertype(L, 1, 0);
  if (self) {
    tolua_pushusertype(L, self->region, "region");
    return 1;
  }
  return 0;
}

static int tolua_ship_set_region(lua_State* L)
{
  ship* self = (ship*) tolua_tousertype(L, 1, 0);
  region * r = (region*) tolua_tousertype(L, 1, 0);
  if (self) {
    move_ship(self, self->region, r, NULL);
  }
  return 0;
}

static int tolua_ship_set_name(lua_State* L)
{
  ship* self = (ship*)tolua_tousertype(L, 1, 0);
  ship_setname(self, tolua_tostring(L, 2, 0));
  return 0;
}

static int 
tolua_ship_get_units(lua_State* L)
{
  ship * self = (ship *)tolua_tousertype(L, 1, 0);
  unit ** unit_ptr = (unit**)lua_newuserdata(L, sizeof(unit *));
  unit * u = self->region->units;

  while (u && u->ship!=self) u = u->next;
  luaL_getmetatable(L, "unit");
  lua_setmetatable(L, -2);

  *unit_ptr = u;

  lua_pushcclosure(L, tolua_unitlist_nexts, 1);
  return 1;
}

static int
tolua_ship_get_objects(lua_State* L)
{
  ship * self = (ship *)tolua_tousertype(L, 1, 0);
  tolua_pushusertype(L, (void*)&self->attribs, "hashtable");
  return 1;
}

static int
tolua_ship_create(lua_State* L)
{
  region * r = (region *)tolua_tousertype(L, 1, 0);
  const char * sname = tolua_tostring(L, 2, 0);
  if (sname) {
    const ship_type * stype = st_find(sname);
    if (stype) {
      ship * sh = new_ship(stype, NULL, r);
      sh->size = stype->construction->maxsize;
      tolua_pushusertype(L, (void*)sh, "ship");
      return 1;
    }
  }
  return 0;
}

static int

tolua_ship_tostring(lua_State *L)
{
  ship * self = (ship *)tolua_tousertype(L, 1, 0);
  lua_pushstring(L, shipname(self));
  return 1;
}

void
tolua_ship_open(lua_State* L)
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
      tolua_variable(L, "name", tolua_ship_get_name, tolua_ship_set_name);
      tolua_variable(L, "units", tolua_ship_get_units, NULL);
      tolua_variable(L, "region", tolua_ship_get_region, tolua_ship_set_region);
#ifdef TODO
      .property("type", &ship_gettype)
      .property("weight", &ship_getweight)
      .property("capacity", &ship_getcapacity)
      .property("maxsize", &ship_maxsize)
      .property("region", &ship_getregion, &ship_setregion)
      .def_readwrite("damage", &ship::damage)
      .def_readwrite("size", &ship::size)
      .def_readwrite("coast", &ship::coast)
#endif
      tolua_variable(L, "objects", tolua_ship_get_objects, 0);

      tolua_function(L, "create", tolua_ship_create);
    }
    tolua_endmodule(L);
  }
  tolua_endmodule(L);
}
