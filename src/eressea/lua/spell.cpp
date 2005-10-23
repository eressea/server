#include <config.h>
#include <eressea.h>
#include "list.h"

// kernel includes
#include <kernel/equipment.h>
#include <kernel/magic.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

// util includes
#include <util/functions.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static const char *
spell_getschool(const spell& sp)
{
  return magietypen[sp.magietyp];
}

static lua_State * luaState;

int 
call_spell(castorder *co)
{
  const char * fname = co->sp->sname;
  unit * mage = (unit*)co->magician;
  int retval = -1;

  if (co->familiar) {
    mage = co->familiar;
  }

  try {
    retval = luabind::call_function<int>(luaState, fname, co->rt, mage, co->level, co->force);
  }
  catch (luabind::error& e) {
    lua_State* L = e.state();
    const char* error = lua_tostring(L, -1);
    log_error(("An exception occured while %s tried to call '%s': %s.\n",
               unitname(mage), fname, error));
    lua_pop(L, 1);
    std::terminate();
  }
  return retval;
}

static void
init_familiar(unit * u)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "initfamiliar_%s", u->race->_name[0]);

  luabind::object globals = luabind::get_globals(luaState);
  luabind::object fun = globals.at(fname);
  if (fun.is_valid()) {
    if (fun.type()!=LUA_TFUNCTION) {
      log_error(("Lua global object %s is not a function, type is %u\n", fname, fun.type()));
    } else {
      try {
        luabind::call_function<int>(luaState, fname, u);
      }
      catch (luabind::error& e) {
        lua_State* L = e.state();
        const char* error = lua_tostring(L, -1);
        log_error(("An exception occured while %s tried to call '%s': %s.\n",
          unitname(u), fname, error));
        lua_pop(L, 1);
        std::terminate();
      }
    }
  }

  snprintf(fname, sizeof(fname), "%s_familiar", u->race->_name[0]);
  equip_unit(u, get_equipment(fname));
}

void
bind_spell(lua_State * L) 
{
  luaState = L;
  module(L)[
    class_<struct spell>("spell")
      .def_readonly("name", &spell::sname)
      .def_readonly("level", &spell::level)
      .property("school", &spell_getschool)
  ];
  register_function((pf_generic)&call_spell, "luaspell");
  register_function((pf_generic)&init_familiar, "luafamiliar");
}
