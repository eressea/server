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
#include <tolua++.h>

int tolua_shiplist_next(lua_State *tolua_S)
{
  ship** ship_ptr = (ship **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  ship * u = *ship_ptr;
  if (u != NULL) {
    tolua_pushusertype(tolua_S, (void*)u, "ship");
    *ship_ptr = u->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int
tolua_ship_get_id(lua_State* tolua_S)
{
  ship * self = (ship *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->no);
  return 1;
}

static int tolua_ship_get_name(lua_State* tolua_S)
{
  ship* self = (ship*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, ship_getname(self));
  return 1;
}

static int tolua_ship_set_name(lua_State* tolua_S)
{
  ship* self = (ship*)tolua_tousertype(tolua_S, 1, 0);
  ship_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int 
tolua_ship_get_units(lua_State* tolua_S)
{
  ship * self = (ship *)tolua_tousertype(tolua_S, 1, 0);
  unit ** unit_ptr = (unit**)lua_newuserdata(tolua_S, sizeof(unit *));
  unit * u = self->region->units;

  while (u && u->ship!=self) u = u->next;
  luaL_getmetatable(tolua_S, "unit");
  lua_setmetatable(tolua_S, -2);

  *unit_ptr = u;

  lua_pushcclosure(tolua_S, tolua_unitlist_nexts, 1);
  return 1;
}

static int
tolua_ship_get_objects(lua_State* tolua_S)
{
  ship * self = (ship *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, (void*)&self->attribs, "hashtable");
  return 1;
}

static int
tolua_ship_create(lua_State* tolua_S)
{
  region * r = (region *)tolua_tousertype(tolua_S, 1, 0);
  const char * sname = tolua_tostring(tolua_S, 2, 0);
  const ship_type * stype = st_find(sname);
  ship * sh = new_ship(stype, NULL, r);
  sh->size = stype->construction->maxsize;
  tolua_pushusertype(tolua_S, (void*)sh, "ship");
  return 1;
}

static int
tolua_ship_tostring(lua_State *tolua_S)
{
  ship * self = (ship *)tolua_tousertype(tolua_S, 1, 0);
  lua_pushstring(tolua_S, shipname(self));
  return 1;
}

void
tolua_ship_open(lua_State* tolua_S)
{
  /* register user types */
  tolua_usertype(tolua_S, "ship");

  tolua_module(tolua_S, NULL, 0);
  tolua_beginmodule(tolua_S, NULL);
  {
    tolua_cclass(tolua_S, "ship", "ship", "", NULL);
    tolua_beginmodule(tolua_S, "ship");
    {
      tolua_function(tolua_S, "__tostring", tolua_ship_tostring);
      tolua_variable(tolua_S, "id", tolua_ship_get_id, NULL);
      tolua_variable(tolua_S, "name", tolua_ship_get_name, tolua_ship_set_name);
      tolua_variable(tolua_S, "units", tolua_ship_get_units, NULL);
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
      tolua_variable(tolua_S, "objects", tolua_ship_get_objects, 0);

      tolua_function(tolua_S, "create", tolua_ship_create);
    }
    tolua_endmodule(tolua_S);
  }
  tolua_endmodule(tolua_S);
}
