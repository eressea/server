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
#include "bind_region.h"
#include "bind_unit.h"
#include "bind_ship.h"
#include "bind_building.h"

#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/item.h>
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>

#include <attributes/key.h>

#include <util/attrib.h>
#include <util/base36.h>
#include <util/language.h>

#include <lua.h>
#include <tolua.h>

int tolua_regionlist_next(lua_State *L)
{
  region** region_ptr = (region **)lua_touserdata(L, lua_upvalueindex(1));
  region * u = *region_ptr;
  if (u != NULL) {
    tolua_pushusertype(L, (void*)u, "region");
    *region_ptr = u->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int
tolua_region_get_id(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->uid);
  return 1;
}

static int
tolua_region_get_x(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->x);
  return 1;
}

static int
tolua_region_get_y(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->y);
  return 1;
}

static int
tolua_region_get_terrain(lua_State* L)
{
  region* self = (region*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, self->terrain->_name);
  return 1;
}

static int tolua_region_get_info(lua_State* L)
{
  region* self = (region*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, region_getinfo(self));
  return 1;
}

static int tolua_region_set_info(lua_State* L)
{
  region* self = (region*)tolua_tousertype(L, 1, 0);
  region_setinfo(self, tolua_tostring(L, 2, 0));
  return 0;
}


static int tolua_region_get_name(lua_State* L)
{
  region* self = (region*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, region_getname(self));
  return 1;
}

static int tolua_region_set_name(lua_State* L)
{
  region* self = (region*)tolua_tousertype(L, 1, 0);
  region_setname(self, tolua_tostring(L, 2, 0));
  return 0;
}


static int tolua_region_get_flag(lua_State* L)
{
  region* self = (region*)tolua_tousertype(L, 1, 0);
  int bit = (int)tolua_tonumber(L, 2, 0);

  lua_pushboolean(L, (self->flags & (1<<bit)));
  return 1;
}

static int tolua_region_get_adj(lua_State* L)
{
  region* self = (region*)tolua_tousertype(L, 1, 0);
  direction_t dir = (direction_t)tolua_tonumber(L, 2, 0);

  tolua_pushusertype(L, (void*)r_connect(self, dir), "region");
  return 1;
}

static int tolua_region_set_flag(lua_State* L)
{
  region* self = (region*)tolua_tousertype(L, 1, 0);
  int bit = (int)tolua_tonumber(L, 2, 0);
  int set = tolua_toboolean(L, 3, 0);

  if (set) self->flags |= (1<<bit);
  else self->flags &= ~(1<<bit);
  return 0;
}

static int
tolua_region_get_resourcelevel(lua_State* L)
{
  region * r = (region *)tolua_tousertype(L, 1, 0);
  const char * type = tolua_tostring(L, 2, 0);
  const resource_type * rtype = rt_find(type);
  if (rtype!=NULL) {
    const rawmaterial * rm;
    for (rm=r->resources;rm;rm=rm->next) {
      if (rm->type->rtype==rtype) {
        tolua_pushnumber(L, (lua_Number)rm->level);
        return 1;
      }
    }
  }
  return 0;
}

static int
tolua_region_get_resource(lua_State* L)
{
  region * r = (region *)tolua_tousertype(L, 1, 0);
  const char * type = tolua_tostring(L, 2, 0);
  const resource_type * rtype = rt_find(type);
  int result = 0;

  if (!rtype) {
    if (strcmp(type, "seed")==0) result = rtrees(r, 0);
    if (strcmp(type, "sapling")==0) result = rtrees(r, 1);
    if (strcmp(type, "tree")==0) result = rtrees(r, 2);
    if (strcmp(type, "grave")==0) result = deathcount(r);
    if (strcmp(type, "chaos")==0) result = chaoscount(r);
  } else {
    result = region_getresource(r, rtype);
  }

  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}

static int
tolua_region_set_resource(lua_State* L)
{
  region * r = (region *)tolua_tousertype(L, 1, 0);
  const char * type = tolua_tostring(L, 2, 0);
  int value = (int)tolua_tonumber(L, 3, 0);
  const resource_type * rtype = rt_find(type);

  if (rtype!=NULL) {
    region_setresource(r, rtype, value);
  } else {
    if (strcmp(type, "seed")==0) {
      rsettrees(r, 0, value);
    } else if (strcmp(type, "sapling")==0) {
      rsettrees(r, 1, value);
    } else if (strcmp(type, "tree")==0) {
      rsettrees(r, 2, value);
    } else if (strcmp(type, "grave")==0) {
      int fallen = value-deathcount(r);
      deathcounts(r, fallen);
    } else if (strcmp(type, "chaos")==0) {
      int fallen = value-chaoscount(r);
      chaoscounts(r, fallen);
    }
  }
  return 0;
}

static int
tolua_region_get_objects(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  tolua_pushusertype(L, (void*)&self->attribs, "hashtable");
  return 1;
}

static int
tolua_region_create(lua_State* L)
{
  int x = (int)tolua_tonumber(L, 1, 0);
  int y = (int)tolua_tonumber(L, 2, 0);
  const char * tname = tolua_tostring(L, 3, 0);

  const terrain_type * terrain = get_terrain(tname);
  region * r = findregion(x, y);
  region * result = r;

  if (terrain==NULL) {
    if (r!=NULL) {
      if (r->units!=NULL) {
        /* TODO: error message */
        result = NULL;
      }
    }
  }
  if (r==NULL) {
    result = new_region(x, y, 0);
  }
  if (result) {
    terraform_region(result, terrain);
  }
  fix_demand(result);

  tolua_pushusertype(L, result, "region");
  return 1;
}

static int tolua_region_get_units(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  unit ** unit_ptr = (unit**)lua_newuserdata(L, sizeof(unit *));

  luaL_getmetatable(L, "unit");
  lua_setmetatable(L, -2);

  *unit_ptr = self->units;

  lua_pushcclosure(L, tolua_unitlist_next, 1);
  return 1;
}

static int tolua_region_get_buildings(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  building ** building_ptr = (building**)lua_newuserdata(L, sizeof(building *));

  luaL_getmetatable(L, "building");
  lua_setmetatable(L, -2);

  *building_ptr = self->buildings;

  lua_pushcclosure(L, tolua_buildinglist_next, 1);
  return 1;
}

static int tolua_region_get_ships(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  ship ** ship_ptr = (ship**)lua_newuserdata(L, sizeof(ship *));

  luaL_getmetatable(L, "ship");
  lua_setmetatable(L, -2);

  *ship_ptr = self->ships;

  lua_pushcclosure(L, tolua_shiplist_next, 1);
  return 1;
}

static int tolua_region_get_age(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);

  if (self) {
    lua_pushnumber(L, self->age);
    return 1;
  }
  return 0;
}

static int
tolua_region_getkey(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  const char * name = tolua_tostring(L, 2, 0);

  int flag = atoi36(name);
  attrib * a = find_key(self->attribs, flag);
  lua_pushboolean(L, a!=NULL);

  return 1;
}

static int
tolua_region_setkey(lua_State* L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  const char * name = tolua_tostring(L, 2, 0);
  int value = tolua_toboolean(L, 3, 0);

  int flag = atoi36(name);
  attrib * a = find_key(self->attribs, flag);
  if (a==NULL && value) {
    add_key(&self->attribs, flag);
  } else if (a!=NULL && !value) {
    a_remove(&self->attribs, a);
  }
  return 0;
}

static int
tolua_region_tostring(lua_State *L)
{
  region * self = (region *)tolua_tousertype(L, 1, 0);
  lua_pushstring(L, regionname(self, NULL));
  return 1;
}

void
tolua_region_open(lua_State* L)
{
  /* register user types */
  tolua_usertype(L, "region");

  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_cclass(L, "region", "region", "", NULL);
    tolua_beginmodule(L, "region");
    {
      tolua_function(L, "create", tolua_region_create);
      tolua_function(L, "__tostring", tolua_region_tostring);

      tolua_variable(L, "id", tolua_region_get_id, NULL);
      tolua_variable(L, "x", tolua_region_get_x, NULL);
      tolua_variable(L, "y", tolua_region_get_y, NULL);
      tolua_variable(L, "name", tolua_region_get_name, tolua_region_set_name);
      tolua_variable(L, "info", tolua_region_get_info, tolua_region_set_info);
      tolua_variable(L, "units", tolua_region_get_units, NULL);
      tolua_variable(L, "ships", tolua_region_get_ships, NULL);
      tolua_variable(L, "age", tolua_region_get_age, NULL);
      tolua_variable(L, "buildings", tolua_region_get_buildings, NULL);
      tolua_variable(L, "terrain", tolua_region_get_terrain, NULL);
      tolua_function(L, "get_resourcelevel", tolua_region_get_resourcelevel);
      tolua_function(L, "get_resource", tolua_region_get_resource);
      tolua_function(L, "set_resource", tolua_region_set_resource);
      tolua_function(L, "get_flag", tolua_region_get_flag);
      tolua_function(L, "set_flag", tolua_region_set_flag);
      tolua_function(L, "next", tolua_region_get_adj);

      tolua_function(L, "get_key", tolua_region_getkey);
      tolua_function(L, "set_key", tolua_region_setkey);
#if 0
      .property("owner", &lua_region_getowner, &lua_region_setowner)
      .property("herbtype", &region_getherbtype, &region_setherbtype)
      .def("add_notice", &region_addnotice)
      .def("add_direction", &region_adddirection)
      .def("move", &region_move)
      .def("get_road", &region_getroad)
      .def("set_road", &region_setroad)
      .def("next", &region_next)
      .def("add_item", &region_additem)
      .property("items", &region_items, return_stl_iterator)
      .property("plane_id", &region_plane)
#endif
      tolua_variable(L, "objects", tolua_region_get_objects, 0);
    }
    tolua_endmodule(L);
  }
  tolua_endmodule(L);
}
