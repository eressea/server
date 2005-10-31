#include <config.h>
#include <eressea.h>

// kernel includes
#include <kernel/unit.h>
#include <kernel/item.h>
#include <util/attrib.h>

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
  lua_State * L = (lua_State *)global.vm_state;
  int retval;
  const char * iname = itype->rtype->_name[0];

  assert(u!=NULL);
  strcat(strcpy(fname, iname), "_use");

  {
    luabind::object globals = luabind::globals(L);
    if (type(globals[fname])!=LUA_TFUNCTION) return -1;
  }

  retval = luabind::call_function<int>(L, fname, *u, amount);
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

void
bind_item(lua_State * L) 
{
  module(L)[
    def("register_item", &item_register)
  ];
}
