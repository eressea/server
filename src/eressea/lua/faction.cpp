#include <config.h>
#include <eressea.h>
#include "list.h"

// kernel includes
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <modules/alliance.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static eressea::list<faction>
get_factions(void) {
  return eressea::list<faction>(factions);
}

class factionunit {
public:
  static unit * next(unit * node) { return node->nextF; }
  static unit * value(unit * node) { return node; }
};

static eressea::list<unit, unit, factionunit>
faction_units(const faction& f)
{
  return eressea::list<unit, unit, factionunit>(f.units);
}

static void
faction_setalliance(faction& f, alliance * team)
{
  if (f.alliance==0) setalliance(&f, team);
}

static alliance *
faction_getalliance(const faction& f)
{
  return f.alliance;
}



void
bind_faction(lua_State * L) 
{
  module(L)[
    def("factions", &get_factions, return_stl_iterator),
    def("get_faction", &findfaction),

    class_<struct faction>("faction")
    .def_readonly("name", &faction::name)
    .def_readonly("id", &faction::no)
    .property("units", &faction_units, return_stl_iterator)
    .property("alliance", &faction_getalliance, &faction_setalliance)
  ];
}
