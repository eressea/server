#include <config.h>
#include <kernel/eressea.h>

#include "bindings.h"

#include <attributes/key.h>
#include <modules/autoseed.h>
#include <modules/score.h>

// kernel includes
#include <kernel/alliance.h>
#include <kernel/calendar.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/reports.h>
#include <kernel/save.h>
#include <kernel/skill.h>
#include <kernel/teleport.h>
#include <kernel/unit.h>

// lua includes
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

// util includes
#include <util/attrib.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/base36.h>
#include <util/rand.h>
#include <util/rng.h>

#include <cstring>
#include <ctime>

using namespace luabind;

static int
lua_addequipment(const char * eqname, const char * iname, const char * value)
{
  if (iname==NULL) return -1;
  const struct item_type * itype = it_find(iname);
  if (itype==NULL) return -1;
  equipment_setitem(create_equipment(eqname), itype, value);
  return 0;
}

static int
get_turn(void)
{
  return turn;
}

static int
get_nmrs(int n)
{
  if (n<=NMRTimeout()) {
    if (nmrs==NULL) update_nmrs();
    return nmrs[n];
  }
  return 0;
}

static int
find_plane_id(const char * name)
{
  plane * pl = getplanebyname(name);
  return pl?pl->id:0;
}

static bool
get_key(const char * name)
{
  int flag = atoi36(name);
  attrib * a = find_key(global.attribs, flag);
  return (a!=NULL);
}

static void
set_key(const char * name, bool value)
{
  int flag = atoi36(name);
  attrib * a = find_key(global.attribs, flag);
  if (a==NULL && value) {
    add_key(&global.attribs, flag);
  } else if (a!=NULL && !value) {
    a_remove(&global.attribs, a);
  }
}

static const char *
get_gamename(void)
{
  return global.gamename;
}

static void
lua_setstring(const char * lname, const char * key, const char * str)
{
  struct locale * lang = find_locale(lname);
  locale_setstring(lang, key, str);
}

static const char *
lua_getstring(const char * lname, const char * key)
{
  if (key) {
    struct locale * lang = find_locale(lname);
    return (const char*)locale_getstring(lang, key);
  }
  return NULL;
}

#define ISLANDSIZE 20
#define TURNS_PER_ISLAND 5
static void
lua_autoseed(const char * filename, bool new_island)
{
  newfaction * players = read_newfactions(filename);
  if (players!=NULL) {
    while (players) {
      int n = listlen(players);
      int k = (n+ISLANDSIZE-1)/ISLANDSIZE;
      k = n / k;
      n = autoseed(&players, k, new_island?0:TURNS_PER_ISLAND);
      if (n==0) {
        break;
      }
    }
  }
}

#ifdef LUABIND_NO_EXCEPTIONS
static void
error_callback(lua_State * L)
{
}
#endif

static int
get_direction(const char * name)
{
  for (int i=0;i!=MAXDIRECTIONS;++i) {
    if (strcasecmp(directions[i], name)==0) return i;
  }
  return NODIRECTION;
}

static void
lua_equipunit(unit * u, const char * eqname)
{
  equip_unit(u, get_equipment(eqname));
}

static void
lua_learnskill(unit * u, const char * skname, float chances)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    learn_skill(u, sk, chances);
  }
}

bool
is_function(struct lua_State * luaState, const char * fname)
{
  object g = globals(luaState);
  object fun = g[fname];
  if (fun.is_valid()) {
    if (type(fun)==LUA_TFUNCTION) {
      return true;
    }
    if (type(fun)!=LUA_TNIL) {
      log_warning(("Lua global object %s is not a function, type is %u\n", fname, type(fun)));
    }
  }
  return false;
}

static const char *
get_season(int turnno)
{
  gamedate gd;
  get_gamedate(turnno, &gd);
  return seasonnames[gd.season];
}

void
bind_eressea(lua_State * L)
{
  module(L)[
    def("atoi36", &atoi36),
    def("itoa36", &itoa36),
    def("dice_roll", &dice_rand),
    def("equipment_setitem", &lua_addequipment),
    def("get_turn", &get_turn),
    def("get_season", &get_season),
    def("get_nmrs", &get_nmrs),
    def("remove_empty_units", &remove_empty_units),

    def("update_subscriptions", &update_subscriptions),
    def("update_scores", &score),

    def("equip_unit", &lua_equipunit),
    def("learn_skill", &lua_learnskill),

    /* map making */
    def("autoseed", lua_autoseed),

    /* string to enum */
    def("direction", &get_direction),

    /* localization: */
    def("set_string", &lua_setstring),
    def("get_string", &lua_getstring),

    def("set_key", &set_key),
    def("get_key", &get_key),

    def("get_gamename", &get_gamename),
    /* planes not really implemented */
    def("get_plane_id", &find_plane_id)
  ];
#ifdef LUABIND_NO_EXCEPTIONS
  luabind::set_error_callback(error_callback);
#endif
}
