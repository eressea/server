#include <config.h>
#include <eressea.h>
#include "bindings.h"

// kernel includes
#include <ship.h>
#include <region.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;
using namespace eressea;

void
bind_ship(lua_State * L) 
{
  module(L)[
    def("get_ship", &findship),

    class_<struct ship>("ship")
    .def_readonly("name", &ship::name)
    .def_readonly("region", &ship::region)
    .def_readonly("id", &ship::no)
    .def_readonly("info", &ship::display)
    .def_readwrite("damage", &ship::damage)
    .def_readwrite("size", &ship::size)
  ];
}
