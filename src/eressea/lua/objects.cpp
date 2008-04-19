#include <config.h>
#include <kernel/eressea.h>
#include "objects.h"

#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/ship.h>
#include <kernel/building.h>
#include <util/attrib.h>

// lua includes
#pragma warning (push)
#pragma warning (disable: 4127)
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>
#pragma warning (pop)

#include <string>

using namespace luabind;

namespace eressea {

  object
  objects::get(const char * name) {
    lua_State * L = (lua_State *)global.vm_state;
    attrib * a = a_find(*mAttribPtr, &at_object);
    for (;a && a->type==&at_object;a=a->next) {
      if (strcmp(object_name(a), name)==0) {
        variant val;
        object_type type;

        object_get(a, &type, &val);
        switch (type) {
          case TNONE:
            break;
          case TINTEGER:
            return object(L, val.i);
          case TREAL:
            return object(L, val.f);
          case TREGION:
            return object(L, (region*)val.v);
          case TBUILDING:
            return object(L, (building*)val.v);
          case TUNIT:
            return object(L, (unit*)val.v);
          case TSHIP:
            return object(L, (ship*)val.v);
          case TSTRING: 
            return object(L, std::string((const char*)val.v));
          default:
            assert(!"not implemented");
        }
      }
    }
#if LUABIND_BETA<7 || (LUABIND_BETA==7 && LUABIND_DEVEL<2)
    return object(L); // nil
#else
    return object(); // nil
#endif
  }

  static void set_object(attrib **attribs, const char * name, object_type type, variant val) {
    attrib * a = a_find(*attribs, &at_object);
    for (;a && a->type==&at_object;a=a->next) {
      if (strcmp(object_name(a), name)==0) {
        object_set(a, type, val);
        return;
      }
    }
    a = a_add(attribs, object_create(name, type, val));
  }

  template<> void
  objects::set<int, TINTEGER>(const char * name, int value) {
    variant val = { 0 };
    val.i = value;
    set_object(mAttribPtr, name, TINTEGER, val);
  }

  template<> void
  objects::set<double, TREAL>(const char * name, double value) {
    variant val = { 0 };
    val.f = (float)value;
    set_object(mAttribPtr, name, TREAL, val);
  }

  template<> void 
  objects::set<const char *, TSTRING>(const char * name, const char * value) {
    variant val = { 0 };
    val.v = strdup(value);
    set_object(mAttribPtr, name, TSTRING, val);
  }

  template <class V, object_type T> void 
  objects::set(const char * name, V value) {
    variant val = { 0 };
    val.v = &value;
    set_object(mAttribPtr, name, T, val);
  }
}

void
bind_objects(struct lua_State * L)
{
  using namespace eressea;

  module(L)[
    class_<objects>("objects")
      .def("get", &objects::get)
      .def("set", (void(objects::*)(const char*, region&))&objects::set<region&, TREGION>)
      .def("set", (void(objects::*)(const char*, unit&))&objects::set<unit&, TUNIT>)
      .def("set", (void(objects::*)(const char*, faction&))&objects::set<faction&, TFACTION>)
      .def("set", (void(objects::*)(const char*, building&))&objects::set<building&, TBUILDING>)
      .def("set", (void(objects::*)(const char*, ship&))&objects::set<ship&, TSHIP>)
      // POD:
//      .def("set", (void(objects::*)(const char*, int))&objects::set<int, TINTEGER>)
      .def("set", (void(objects::*)(const char*, double))&objects::set<lua_Number, TREAL>)
      .def("set", (void(objects::*)(const char*, const char *))&objects::set<const char *, TSTRING>)
  ];
}
