#include <config.h>
#include <eressea.h>
#include "list.h"
#include "objects.h"
#include "bindings.h"

// kernel includes
#include <building.h>
#include <region.h>
#include <unit.h>

// lua includes
#pragma warning (push)
#pragma warning (disable: 4127)
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#if LUABIND_BETA >= 7
# include <luabind/operator.hpp>
#endif
#pragma warning (pop)

// util includes
#include <util/attrib.h>
#include <util/base36.h>
#include <util/lists.h>
#include <util/log.h>

using namespace luabind;

static building *
add_building(region * r, const char * name)
{
  const building_type * btype = bt_find(name);
  if (btype==NULL) return NULL;
  return new_building(btype, r, NULL);
}

static int
lc_age(struct attrib * a)
{
  building_action * data = (building_action*)a->data.v;
  const char * fname = data->fname;
  const char * fparam = data->param;
  building * b = data->b;
  int retval = -1;

  assert(b!=NULL);
  if (fname==NULL) return -1;

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      if (fparam) {
        retval = luabind::call_function<int>(L, fname, *b, fparam);
      } else {
        retval = luabind::call_function<int>(L, fname, *b);
      }
    }
    catch (luabind::error& e) {
      lua_State* L = e.state();
      const char* error = lua_tostring(L, -1);
      log_error(("An exception occured while %s tried to call '%s': %s.\n",
        buildingname(b), fname, error));
      lua_pop(L, 1);
      std::terminate();
    }
  }
  return retval;
}

static int
building_addaction(building& b, const char * fname, const char * param)
{
  attrib * a = a_add(&b.attribs, a_new(&at_building_action));
  building_action * data = (building_action*)a->data.v;
  data->b = &b;
  data->fname = strdup(fname);
  if (param) data->param = strdup(param);

  return 0;
}

static const char *
building_getinfo(const building& b)
{
  return (const char*)b.display;
}

static void
building_setinfo(building& b, const char * info)
{
  free(b.display);
  b.display = strdup(info);
}

static const char *
building_getname(const building& b)
{
  return (const char *)b.name;
}

static void
building_setname(building& b, const char * name)
{
  free(b.name);
  b.name = strdup(name);
}

static region *
building_getregion(const building& b)
{
  return b.region;
}

static void
building_setregion(building& b, region& r)
{
	choplist(&b.region->buildings, &b);
  addlist(&r.buildings, &b);
  b.region = &r;
}

static std::ostream& 
operator<<(std::ostream& stream, const building& b)
{
  stream << b.name;
  stream << " (" << itoa36(b.no) << ")";
  stream << ", " << b.type->_name;
  stream << " size " << b.size;
  return stream;
}

static bool 
operator==(const building& a, const building&b)
{
  return a.no==b.no;
}

class buildingunit {
public:
  static unit * next(unit * node) { 
    building * b = node->building;
    do {
      node = node->next;
    } while (node !=NULL && node->building!=b);
    return node;
  }
  static unit * value(unit * node) { return node; }
};


static eressea::list<unit *, unit *, buildingunit>
building_units(const building& b) {
  region * r = b.region;
  unit * u = r->units;
  while (u!=NULL && u->building!=&b) u=u->next;
  return eressea::list<unit *, unit *, buildingunit>(u);
}

const char *
building_gettype(const building& b) {
  return b.type->_name;
}

void
bind_building(lua_State * L) 
{
  at_building_action.age = lc_age;
  module(L)[
    def("get_building", &findbuilding),
    def("add_building", &add_building),

    class_<struct building>("building")
    .def(self == building())
    .def(tostring(self))
    .property("name", &building_getname, &building_setname)
    .property("info", &building_getinfo, &building_setinfo)
    .property("units", &building_units, return_stl_iterator)
    .property("region", &building_getregion, &building_setregion)
    .property("type", &building_gettype)
    .def_readonly("id", &building::no)
    .def_readwrite("size", &building::size)
    .def("add_action", &building_addaction)
    .property("objects", &eressea::get_objects<building>)
  ];
}
