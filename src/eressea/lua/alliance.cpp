#include <config.h>
#include <eressea.h>
#include "list.h"

// kernel includes
#include <kernel/region.h>
#include <modules/alliance.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static eressea::list<alliance>
get_alliances(void) {
  return eressea::list<alliance>(alliances);
}

void
bind_alliance(lua_State * L) 
{
  module(L)[
    def("alliances", &get_alliances, return_stl_iterator),
    def("get_alliance", &findalliance),

    class_<struct alliance>("alliance")
    .def_readonly("name", &alliance::name)
    .def_readonly("id", &alliance::id)
  ];
}
