#include <config.h>
#include <cstring>
#include <eressea.h>

// kernel includes
#include <modules/alliance.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/region.h>
#include <util/language.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static faction *
add_faction(const char * email, const char * passwd, const char * racename, const char * lang)
{
  const race * frace = findrace(racename, default_locale);
  locale * loc = find_locale(lang);
  faction * f = addfaction(email, passwd, frace, loc, 0);
  return f;
}

static unit *
add_unit(faction * f, region * r)
{
  if (f->units==NULL) return addplayer(r, f);
  return createunit(r, f, 0, f->race);
}

static alliance *
add_alliance(int id, const char * name)
{
  return makealliance(id, name);
}

void
bind_eressea(lua_State * L) 
{
  module(L)[
    def("add_unit", &add_unit),
    def("add_faction", &add_faction),
    def("add_alliance", &add_alliance)
  ];
}
