#include <config.h>
#include <eressea.h>
#include "list.h"

// kernel includes
#include <kernel/faction.h>
#include <kernel/unit.h>
#ifdef ALLIANCES
# include <modules/alliance.h>
#endif

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static faction *
add_faction(const char * email, const char * racename, const char * lang)
{
  const race * frace = findrace(racename, default_locale);
  if (frace==NULL) frace = findrace(racename, find_locale("de"));
  if (frace==NULL) frace = findrace(racename, find_locale("en"));
  if (frace==NULL) return NULL;
  locale * loc = find_locale(lang);
  faction * f = addfaction(email, NULL, frace, loc, 0);
  return f;
}

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

#ifdef ALLIANCES
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
#endif

void
bind_faction(lua_State * L) 
{
  module(L)[
    def("factions", &get_factions, return_stl_iterator),
    def("get_faction", &findfaction),
    def("add_faction", &add_faction),

    class_<struct faction>("faction")
    .def_readonly("name", &faction::name)
    .def_readonly("password", &faction::passw)
    .def_readonly("email", &faction::email)
    .def_readonly("id", &faction::no)
    .def_readwrite("subscription", &faction::subscription)
    .def_readwrite("lastturn", &faction::lastorders)
    .property("units", &faction_units, return_stl_iterator)
#ifdef ALLIANCES
    .property("alliance", &faction_getalliance, &faction_setalliance)
#endif
  ];
}
