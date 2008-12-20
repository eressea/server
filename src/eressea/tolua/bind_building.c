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
#include "bind_building.h"
#include "bind_unit.h"

#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/region.h>

#include <lua.h>
#include <tolua.h>

static int
tolua_building_addaction(lua_State* tolua_S)
{
  building* self = (building*)tolua_tousertype(tolua_S, 1, 0);
  const char * fname = tolua_tostring(tolua_S, 2, 0);
  const char * param = tolua_tostring(tolua_S, 3, 0);

  building_addaction(self, fname, param);

  return 0;
}

static int
tolua_building_get_objects(lua_State* tolua_S)
{
  building * self = (building *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, (void*)&self->attribs, "hashtable");
  return 1;
}

static int tolua_building_get_name(lua_State* tolua_S)
{
  building* self = (building*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, building_getname(self));
  return 1;
}

static int tolua_building_set_name(lua_State* tolua_S)
{
  building* self = (building*)tolua_tousertype(tolua_S, 1, 0);
  building_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int 
tolua_building_get_units(lua_State* tolua_S)
{
  building * self = (building *)tolua_tousertype(tolua_S, 1, 0);
  unit ** unit_ptr = (unit**)lua_newuserdata(tolua_S, sizeof(unit *));
  unit * u = self->region->units;

  while (u && u->building!=self) u = u->next;
  luaL_getmetatable(tolua_S, "unit");
  lua_setmetatable(tolua_S, -2);

  *unit_ptr = u;

  lua_pushcclosure(tolua_S, tolua_unitlist_nextb, 1);
  return 1;
}

static int
tolua_building_get_id(lua_State* tolua_S)
{
  building * self = (building *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->no);
  return 1;
}

static int
tolua_building_create(lua_State* tolua_S)
{
  region * r = (region *)tolua_tousertype(tolua_S, 1, 0);
  const char * bname = tolua_tostring(tolua_S, 2, 0);
  const building_type * btype = bt_find(bname);
  building * b = new_building(btype, r, NULL);
  tolua_pushusertype(tolua_S, (void*)b, "building");
  return 1;
}


void
tolua_building_open(lua_State* tolua_S)
{
  /* register user types */
  tolua_usertype(tolua_S, "building");
  tolua_usertype(tolua_S, "building_list");

  tolua_module(tolua_S, NULL, 0);
  tolua_beginmodule(tolua_S, NULL);
  {
    tolua_cclass(tolua_S, "building", "building", "", NULL);
    tolua_beginmodule(tolua_S, "building");
    {
      tolua_variable(tolua_S, "id", tolua_building_get_id, NULL);
      tolua_variable(tolua_S, "name", tolua_building_get_name, tolua_building_set_name);
      tolua_variable(tolua_S, "units", tolua_building_get_units, NULL);
      tolua_function(tolua_S, "add_action", tolua_building_addaction);

      tolua_variable(tolua_S, "objects", tolua_building_get_objects, 0);

      tolua_function(tolua_S, "create", tolua_building_create);
    }
    tolua_endmodule(tolua_S);
  }
  tolua_endmodule(tolua_S);
}
