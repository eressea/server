#include <config.h>
#include <eressea.h>

#include "list.h"

// kernel includes
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/alliance.h>

// lua includes
#pragma warning (push)
#pragma warning (disable: 4127)
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#include <luabind/object.hpp>
#pragma warning (pop)

using namespace luabind;

class factionlist_iterator {
public:
  static faction_list * next(faction_list * node) { return node->next; }
  static faction * value(faction_list * node) { return node->data; }
};

static eressea::list<faction *, faction_list *, factionlist_iterator>
alliance_factions(const alliance& al)
{
  return eressea::list<faction *, faction_list *, factionlist_iterator>(al.members);
}

static alliance *
add_alliance(int id, const char * name)
{
  return makealliance(id, name);
}

static eressea::list<alliance *>
get_alliances(void) {
  return eressea::list<alliance *>(alliances);
}

void
bind_alliance(lua_State * L) 
{
  module(L)[
    def("alliances", &get_alliances, return_stl_iterator),
    def("get_alliance", &findalliance),
    def("add_alliance", &add_alliance),
    def("victorycondition", &victorycondition),
    class_<struct alliance>("alliance")
    .def_readonly("name", &alliance::name)
    .def_readonly("id", &alliance::id)
    .property("factions", &alliance_factions, return_stl_iterator)
  ];
}
