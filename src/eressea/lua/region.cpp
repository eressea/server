#include <config.h>
#include <eressea.h>
#include "list.h"

// kernel includes
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/ship.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#ifdef HAVE_LUABIND_B7
# include <luabind/operator.hpp>
#endif

#include <ostream>
using namespace luabind;

static eressea::list<region *>
get_regions(void) {
  return eressea::list<region *>(regions);
}

static eressea::list<unit *>
region_units(const region& r) {
  return eressea::list<unit *>(r.units);
}

static eressea::list<building *>
region_buildings(const region& r) {
  return eressea::list<building *>(r.buildings);
}

static eressea::list<ship *>
region_ships(const region& r) {
  return eressea::list<ship *>(r.ships);
}

static void
region_setname(region& r, const char * name) {
  if (r.land) rsetname((&r), name);
}

static const char *
region_getterrain(const region& r) {
  return terrain[r.terrain].name;
}

static const char *
region_getname(const region& r) {
  if (r.land) return r.land->name;
  return terrain[r.terrain].name;
}

static void
region_setinfo(region& r, const char * info) {
  set_string(&r.display, info);
}

static const char *
region_getinfo(const region& r) {
  return r.display;
}

static int 
region_plane(const region& r)
{
  if (r.planep==NULL) return 0;
  return r.planep->id;
}

static void
region_addnotice(region& r, const char * str)
{
  addmessage(&r, NULL, str, MSG_MESSAGE, ML_IMPORTANT);
}

static std::ostream& 
operator<<(std::ostream& stream, const region& r)
{
  stream << regionname(&r, NULL) << ", " << region_getterrain(r);
  return stream;
}

static bool 
operator==(const region& a, const region&b)
{
  return a.x==b.x && a.y==b.y;
}

static bool
region_getflag(const region& r, int bit)
{
  if (r.flags & (1<<bit)) return true;
  return false;
}

static void
region_setflag(region& r, int bit, bool set)
{
  if (set) r.flags |= (1<<bit);
  else r.flags &= ~(1<<bit);
}

static void
region_setroad(region& r, int dir, lua_Number size)
{
  rsetroad(&r, (direction_t)dir, (int)(terrain[rterrain(&r)].roadreq * size));
}

static lua_Number
region_getroad(region& r, int dir)
{
  lua_Number result = rroad(&r, (direction_t)dir);
  return terrain[rterrain(&r)].roadreq / result;
}

static region *
terraform_region(int x, int y, const char * tname)
{
  terrain_t t;

  if (tname==NULL) {
    t = NOTERRAIN;
  } else {
    for (t=0;t!=MAXTERRAINS;++t) {
      if (strcmp(terrain[t].name, tname)==0) break;
    }
    if (t==MAXTERRAINS) return NULL;
  }

  region * r = findregion(x, y);
  if (t==NOTERRAIN) {
    if (r!=NULL) {
      if (r->units!=NULL) {
        // TODO: error message
        return r; 
      }
      terraform(r, T_FIREWALL);
      // TODO: durch einen NULL-äquivalenten terraintyp ersetzen
    }
    return NULL;
  }
  if (r==NULL) r = new_region(x, y);
  terraform(r, t);
  return r;
}

static region *
region_next(const region& r, int dir)
{
  if (dir<0 || dir >=MAXDIRECTIONS) return NULL;
  return r_connect(&r, (direction_t)dir);
}

void
bind_region(lua_State * L) 
{
  module(L)[
    def("regions", &get_regions, return_stl_iterator),
    def("get_region", &findregion),
    def("terraform", &terraform_region),

    class_<struct region>("region")
    .def(tostring(self))
    .def(self == region())
    .property("name", &region_getname, &region_setname)
    .property("info", &region_getinfo, &region_setinfo)
    .property("terrain", &region_getterrain)
    .def("add_notice", &region_addnotice)

    .def("get_flag", &region_getflag)
    .def("set_flag", &region_setflag)

    .def("get_road", &region_getroad)
    .def("set_road", &region_setroad)

    .def("next", &region_next)
    .def_readonly("x", &region::x)
    .def_readonly("y", &region::y)
    .def_readwrite("age", &region::age)
    .property("plane_id", &region_plane)
    .property("units", &region_units, return_stl_iterator)
    .property("buildings", &region_buildings, return_stl_iterator)
    .property("ships", &region_ships, return_stl_iterator)
  ];
}
