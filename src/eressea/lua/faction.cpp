#include <config.h>
#include <eressea.h>
#include "list.h"

// kernel includes
#include <kernel/faction.h>
#include <kernel/unit.h>
#ifdef ALLIANCES
# include <modules/alliance.h>
#endif

// util includes
#include <util/base36.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

#include <ostream>
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

const char *
faction_locale(const faction& f)
{
  return locale_name(f.locale);
}

static std::ostream& 
operator<<(std::ostream& stream, faction& f)
{
  stream << factionname(&f);
  return stream;
}

static bool 
operator==(const faction& a, const faction&b)
{
  return a.no==b.no;
}

static struct helpmode {
  const char * name;
  int status;
} helpmodes[] = {
  { "money", HELP_MONEY },
  { "fight", HELP_FIGHT },
  { "observe", HELP_OBSERVE },
  { "give", HELP_GIVE },
  { "guard", HELP_GUARD },
  { "stealth", HELP_FSTEALTH },
  { "travel", HELP_TRAVEL },
  { "all", HELP_ALL },
  { NULL, 0 }
};

static int
faction_getpolicy(const faction& a, const faction& b, const char * flag)
{
  int mode;

  for (mode=0;helpmodes[mode].name!=NULL;++mode) {
    if (strcmp(flag, helpmodes[mode].name)==0) {
      return get_alliance(&a, &b) & mode;
    }
  }
  return 0;
}

static void
faction_setpolicy(faction& a, faction& b, const char * flag, bool value)
{
  int mode;

  for (mode=0;helpmodes[mode].name!=NULL;++mode) {
    if (strcmp(flag, helpmodes[mode].name)==0) {
      if (value) set_alliance(&a, &b, get_alliance(&a, &b) | helpmodes[mode].status);
      else set_alliance(&a, &b, get_alliance(&a, &b) & ~helpmodes[mode].status);
      break;
    }
  }
}

void
bind_faction(lua_State * L) 
{
  module(L)[
    def("factions", &get_factions, return_stl_iterator),
    def("get_faction", &findfaction),
    def("add_faction", &add_faction),

    class_<struct faction>("faction")
    .def(tostring(self))
    .def(self == faction())
    .def("set_policy", &faction_setpolicy)
    .def("get_policy", &faction_getpolicy)
    .def_readonly("name", &faction::name)
    .def_readonly("password", &faction::passw)
    .def_readonly("email", &faction::email)
    .def_readonly("id", &faction::no)
    .def_readwrite("age", &faction::age)
    .def_readwrite("subscription", &faction::subscription)
    .def_readwrite("lastturn", &faction::lastorders)
    .property("locale", &faction_locale)
    .property("units", &faction_units, return_stl_iterator)
#ifdef ALLIANCES
    .property("alliance", &faction_getalliance, &faction_setalliance)
#endif
  ];
}
