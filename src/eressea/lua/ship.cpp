#include <config.h>
#include <eressea.h>

// kernel includes
#include <ship.h>
#include <region.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#ifdef HAVE_LUABIND_B7
# include <luabind/operator.hpp>
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

void
bind_ship(lua_State * L) 
{
  module(L)[
    def("get_ship", &findship),

    class_<struct ship>("ship")
    .def(self == ship())
    .def(tostring(self))
    .def_readonly("name", &ship::name)
    .def_readonly("region", &ship::region)
    .def_readonly("id", &ship::no)
    .def_readonly("info", &ship::display)
    .def_readwrite("damage", &ship::damage)
    .def_readwrite("size", &ship::size)
    .def_readwrite("coast", &ship::coast)
  ];
}
