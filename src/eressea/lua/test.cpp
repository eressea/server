#include <config.h>
#include <kernel/eressea.h>

#include "bindings.h"
#include "list.h"

// Lua includes
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

using namespace luabind;

#include <util/language.h>
#include <util/rng.h>
#include <kernel/skill.h>

static const char *
loc_getskill(const char * loc, const char * locstring)
{
  struct locale * lang = find_locale(loc);
  skill_t result = findskill(locstring, lang);
  if (result==NOSKILL) return 0;
  return skillnames[result];
}

static const char *
loc_getkeyword(const char * loc, const char * locstring)
{
  struct locale * lang = find_locale(loc);
  keyword_t result = findkeyword(locstring, lang);
  if (result==NOKEYWORD) return 0;
  return keywords[result];
}

void
bind_test(lua_State * L)
{
  module(L, "test")[
    def("loc_skill", &loc_getskill),
    def("loc_keyword", &loc_getkeyword),
    def("rng_int", &rng_int)
  ];
}
