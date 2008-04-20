#include <config.h>
#include <kernel/eressea.h>
#include "list.h"

// kernel includes
#include <kernel/magic.h>

// lua includes
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
#include <luabind/luabind.hpp>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

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
