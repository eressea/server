#include <config.h>
#include <eressea.h>
#include "event.h"

// kernel includes
#include <kernel/unit.h>

// util includes
#include <util/base36.h>
#include <util/event.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>

using namespace luabind;


const char * 
event::get_type(int i) const 
{ 
  return args[i].type; 
}

struct unit * 
event::get_unit(int i) const 
{
  return (struct unit *)args[i].data; 
}

const char * 
event::get_string(int i) const 
{ 
  return (const char*)args[i].data; 
}

int 
event::get_int(int i) const 
{
  return (int)args[i].data; 
}

void
bind_event(lua_State * L) 
{
  module(L)[
    class_<event>("event")
      .def("message", &event::get_message)
      .def("get_type", &event::get_type)
      .def("get_string", &event::get_string)
      .def("get_unit", &event::get_unit)
      .def("get_int", &event::get_int)
  ];
}
