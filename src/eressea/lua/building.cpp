#include <config.h>
#include <eressea.h>
#include "bindings.h"

// kernel includes
#include <building.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;
using namespace eressea;

void
bind_building(lua_State * L) 
{
  module(L)[
    def("get_building", &findbuilding),

    class_<struct building>("building")
    .def_readonly("name", &building::name)
    .def_readonly("id", &building::no)
    .def_readonly("info", &building::display)
    .def_readwrite("size", &building::size)
  ];
}
