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
#include <kernel/region.h>
#include <kernel/skill.h>
#include <kernel/terrainid.h>
#include <modules/autoseed.h>

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

static void
adamantium_island(region * r)
{
  region_list *rp, *rlist = NULL;
  get_island(r, &rlist);

  for (rp=rlist;rp;rp=rp->next) {
    region * ri = rp->data;
    if (ri->terrain==newterrain(T_MOUNTAIN)) {
      int base = 1 << (rng_int() % 4);
      seed_adamantium(ri, base);
    }
  }
  free_regionlist(rlist);
}

void
bind_test(lua_State * L)
{
  module(L, "test")[
    def("loc_skill", &loc_getskill),
    def("loc_keyword", &loc_getkeyword),
    def("reorder_units", &reorder_units),
    def("adamantium_island", &adamantium_island)
  ];
}
