#include <config.h>
#include <kernel/eressea.h>
#include "objects.h"

// kernel includes
#include <kernel/build.h>
#include <kernel/ship.h>
#include <kernel/region.h>
#include <kernel/move.h>

// lua includes
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#include <luabind/operator.hpp>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

#include <util/base36.h>

#include <ostream>
using namespace luabind;

static std::ostream&
operator<<(std::ostream& stream, const ship& sh)
{
  stream << sh.name;
  stream << " (" << itoa36(sh.no) << ")";
  stream << ", " << sh.type->name;
  stream << " size " << sh.size;
  return stream;
}

static bool
operator==(const ship& a, const ship& sh)
{
  return a.no==sh.no;
}

static ship *
add_ship(region * r, const char * sname)
{
  const ship_type * stype = st_find(sname);
  ship * sh = new_ship(stype, NULL, r);
  sh->size = stype->construction->maxsize;
  return sh;
}

static int
ship_maxsize(const ship * s) {
  return s->type->construction->maxsize;
}

const char *
ship_gettype(const ship * s) {
  return s->type->name[0];
}

int
ship_getweight(const ship * s) {
  int w, c;
  getshipweight(s, &w, &c);
  return w;
}

int
ship_getcapacity(const ship * s) {
  return shipcapacity(s);
}

static void
ship_setregion(ship * sh, region * r)
{
  move_ship(sh, sh->region, r, NULL);
}

static region *
ship_getregion(const ship * sh)
{
  return sh->region;
}

void
bind_ship(lua_State * L)
{
  module(L)[
    def("get_ship", &findship),
    def("add_ship", &add_ship),

    class_<struct ship>("ship")
    .def(self == ship())
    .def(tostring(self))
    .property("type", &ship_gettype)
    .property("weight", &ship_getweight)
    .property("capacity", &ship_getcapacity)
    .property("maxsize", &ship_maxsize)
    .def_readonly("name", &ship::name)
    .property("region", &ship_getregion, &ship_setregion)
    .def_readonly("id", &ship::no)
    .def_readonly("info", &ship::display)
    .def_readwrite("damage", &ship::damage)
    .def_readwrite("size", &ship::size)
    .def_readwrite("coast", &ship::coast)
    .property("objects", &eressea::get_objects<ship>)
  ];
}
