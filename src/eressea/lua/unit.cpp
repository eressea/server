#include <config.h>
#include <eressea.h>
#include "bindings.h"

// kernel includes
#include <region.h>
#include <unit.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;
using namespace eressea;

void
bind_unit(lua_State * L) 
{
  module(L)[
    def("get_unit", &findunit),

    class_<struct unit>("unit")
    .def_readonly("name", &unit::name)
    .def_readonly("region", &unit::region)
    .def_readonly("id", &unit::no)
  ];
}
