#include <config.h>
#include <eressea.h>

#include "bindings.h"

// kernel includes
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/unit.h>

// util includes
#include <util/attrib.h>
#include <util/functions.h>
#include <util/log.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static int
lua_useitem(struct unit * u, const struct item_type * itype,
            int amount, struct order *ord)
{
  char fname[64];
  int retval = -1;
  const char * iname = itype->rtype->_name[0];

  assert(u!=NULL);
  strcat(strcpy(fname, iname), "_use");

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      retval = luabind::call_function<int>(L, fname, u, amount);
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
  return retval;
}

static void
item_register(const char * name, const char * appearance)
{
  resource_type * rtype = (resource_type *)calloc(1, sizeof(resource_type));
  item_type * itype = (item_type *)calloc(1, sizeof(item_type));

  rtype->_name[0] = strdup(name);
  rtype->_name[1] = strcat(strcpy((char*)malloc(strlen(name)+3), name), "_p");
  rtype->_appearance[0] = strdup(appearance);
  rtype->_appearance[1] = strcat(strcpy((char*)malloc(strlen(appearance)+3), appearance), "_p");

  itype->use = lua_useitem;
  itype->rtype = rtype;

  rt_register(rtype);
  it_register(itype);
  init_itemnames();
}

static int
limit_resource(const region * r, const resource_type * rtype)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_limit", rtype->_name[0]);
  int retval = -1;

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      retval = call_function<int>(L, fname, r);
    }
    catch (error& e) {
      lua_State* L = e.state();
      const char* error = lua_tostring(L, -1);
      log_error(("An exception occured while trying to call '%s': %s.\n",
        fname, error));
      lua_pop(L, 1);
      std::terminate();
    }
  }
  return retval;
}

static void
produce_resource(region * r, const resource_type * rtype, int norders)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_produce", rtype->_name[0]);

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      call_function<void>(L, fname, r, norders);
    }
    catch (error& e) {
      lua_State* L = e.state();
      const char* error = lua_tostring(L, -1);
      log_error(("An exception occured while trying to call '%s': %s.\n",
        fname, error));
      lua_pop(L, 1);
      std::terminate();
    }
  }
}

void
bind_item(lua_State * L) 
{
  register_function((pf_generic)produce_resource, "lua_produceresource");
  register_function((pf_generic)limit_resource, "lua_limitresource");

  module(L)[
    def("register_item", &item_register)
  ];
}
