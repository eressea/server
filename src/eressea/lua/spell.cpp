#include <config.h>
#include <kernel/eressea.h>
#include "list.h"

// kernel includes
#include <kernel/magic.h>

// lua includes
#pragma warning (push)
#pragma warning (disable: 4127)
#include <luabind/luabind.hpp>
#pragma warning (pop)

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
