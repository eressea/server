#include <config.h>
#include <eressea.h>
#include "message.h"

// kernel includes
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/unit.h>

// util includes
#include <util/message.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#ifdef HAVE_LUABIND_B7
# include <luabind/operator.hpp>
#endif

#include <ostream>

using namespace luabind;

#define E_OK 0
#define E_INVALID_MESSAGE 1
#define E_INVALID_PARAMETER_NAME 2
#define E_INVALID_PARAMETER_TYPE 3
#define E_INVALID_PARAMETER_VALUE 4

class lua_message {
public:
  lua_message(const char *type) : msg(0), args(0) {
    mtype = mt_find(type);
    if (mtype) {
      args = (variant*)calloc(mtype->nparameters, sizeof(variant));
    }
  }

  ~lua_message() {
    if (msg) msg_release(msg);
    if (mtype) {
      for (int i=0;i!=mtype->nparameters;++i) {
        if (mtype->types[i]->release) {
          mtype->types[i]->release(args[i]);
        }
      }
    }
  }

  int set_resource(const char * param, const char * resname) {
    if (mtype==0) return E_INVALID_MESSAGE;

    int i = get_param(param);
    if (i==mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(mtype->types[i]->name, "resource")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    args[i].v = (void*)rt_find(resname);

    return E_OK;
  }

  int set_unit(const char * param, const unit& u) {
    if (mtype==0) return E_INVALID_MESSAGE;

    int i = get_param(param);
    if (i==mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(mtype->types[i]->name, "unit")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    args[i].v = (void*)&u;

    return E_OK;
  }

  int set_string(const char * param, const char * value) {
    if (mtype==0) return E_INVALID_MESSAGE;

    int i = get_param(param);
    if (i==mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(mtype->types[i]->name, "string")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    variant var;
    var.v = (void*)value;
    args[i] = mtype->types[i]->copy(var);

    return E_OK;
  }

  int set_int(const char * param, int value) {
    if (mtype==0) return E_INVALID_MESSAGE;

    int i = get_param(param);
    if (i==mtype->nparameters) {
      return E_INVALID_PARAMETER_NAME;
    }
    if (strcmp(mtype->types[i]->name, "int")!=0) {
      return E_INVALID_PARAMETER_TYPE;
    }

    args[i].i = value;

    return E_OK;
  }

  int send_faction(faction& f) {
    if (mtype==0) return E_INVALID_MESSAGE;
    if (msg==NULL) {
      msg = msg_create(mtype, args);
    }
    add_message(&f.msgs, msg);
    return E_OK;
  }

protected:
  int get_param(const char * param) {
    for (int i=0;i!=mtype->nparameters;++i) {
      if (strcmp(mtype->pnames[i], param)==0) {
        return i;
      }
    }
    return mtype->nparameters;
  }

  const message_type * mtype;
  message * msg;
  variant * args;
};

static std::ostream& 
operator<<(std::ostream& stream, const lua_message& msg)
{
  stream << "(message object)";
  return stream;
}


void
bind_message(lua_State * L) 
{
  module(L)[
    class_<lua_message>("message")
      .def(constructor<const char *>())
      .def(tostring(self))

      .def("set_unit", &lua_message::set_unit)
      .def("set_resource", &lua_message::set_resource)
      .def("set_int", &lua_message::set_int)
      .def("set_string", &lua_message::set_string)
      .def("send_faction", &lua_message::send_faction)
  ];
}
