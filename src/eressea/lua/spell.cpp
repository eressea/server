#include <config.h>
#include <eressea.h>
#include "list.h"

// Atributes includes
#include <attributes/racename.h>

// kernel includes
#include <kernel/magic.h>

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

void
bind_spell(lua_State * L) 
{
  module(L)[
    class_<struct spell>("spell")
      .def_readonly("name", &spell::sname)
      .def_readonly("level", &spell::level)
      .property("school", &spell_getschool)
  ];
}
