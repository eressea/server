#include <config.h>
#include <eressea.h>
#include "bindings.h"

// kernel includes
#include <region.h>
#include <unit.h>
#include <building.h>
#include <ship.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;
using namespace eressea;

static eressea::list<region>
get_regions(void) {
  return eressea::list<region>(regions);
}

static eressea::list<unit>
region_units(const region& r) {
  return eressea::list<unit>(r.units);
}

static eressea::list<building>
region_buildings(const region& r) {
  return eressea::list<building>(r.buildings);
}

static eressea::list<ship>
region_ships(const region& r) {
  return eressea::list<ship>(r.ships);
}

static void
region_setname(region& r, const char * name) {
  if (r.land) rsetname((&r), name);
}

static const char *
region_getname(const region& r) {
  if (r.land) return r.land->name;
  return r.terrain->name;
}

static void
region_setinfo(region& r, const char * info) {
  if (r.land) set_string(&r.land->display, info);
}

static const char *
region_getinfo(const region& r) {
  if (r.land) return r.land->display;
  return NULL;
}

void
bind_region(lua_State * L) 
{
  module(L)[
    def("get_regions", &get_regions, return_stl_iterator),
    def("get_region", &findregion),

    class_<struct region>("region")
    .property("name", &region_getname, &region_setname)
    .property("info", &region_getinfo, &region_setinfo)
    .def_readonly("x", &region::x)
    .def_readonly("y", &region::y)
    .def_readwrite("age", &region::age)
    .property("units", &region_units, return_stl_iterator)
    .property("buildings", &region_buildings, return_stl_iterator)
    .property("ships", &region_ships, return_stl_iterator)
  ];
}
