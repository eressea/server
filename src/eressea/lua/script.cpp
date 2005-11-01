/* vi: set ts=2:
 +-------------------+  
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea-pbem.de>
 | (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+  

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include "eressea.h"
#include "script.h"

// kernel includes
#include <kernel/equipment.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

// util includes
#include <util/attrib.h>
#include <util/functions.h>

#include <boost/version.hpp>
#include <lua.hpp>
#include <luabind/luabind.hpp>

#include <cstdio>
#include <cstring>

static void 
free_script(attrib * a) {
  if (a->data.v!=NULL) {
    luabind::object * f = (luabind::object *)a->data.v;
    delete f;
  }
}

attrib_type at_script = {
  "script", 
  NULL, free_script, NULL, 
  NULL, NULL, ATF_UNIQUE 
};

int 
call_script(struct unit * u)
{
  const attrib * a = a_findc(u->attribs, &at_script);
  if (a==NULL) a = a_findc(u->race->attribs, &at_script);
  if (a!=NULL && a->data.v!=NULL) {
    luabind::object * func = (luabind::object *)a->data.v;
    try {	
      func->operator()(u);
    }
    catch (luabind::error& e) {
      lua_State* L = e.state();
      const char* error = lua_tostring(L, -1);
      log_error((error));
      lua_pop(L, 1);
      std::terminate();
    }
  }
  return -1;
}

void 
setscript(struct attrib ** ap, void * fptr)
{
  attrib * a = a_find(*ap, &at_script);
  if (a == NULL) {
    a = a_add(ap, a_new(&at_script));
  } else if (a->data.v!=NULL) {
    luabind::object * f = (luabind::object *)a->data.v;
    delete f;
  }
  a->data.v = fptr;
}

/** callback to use lua for spell functions */
static int 
lua_callspell(castorder *co)
{
  const char * fname = co->sp->sname;
  unit * mage = (unit*)co->magician;
  int retval = -1;

  if (co->familiar) {
    mage = co->familiar;
  }

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      retval = luabind::call_function<int>(L, fname, co->rt, mage, co->level, co->force);
    }
    catch (luabind::error& e) {
      lua_State* L = e.state();
      const char* error = lua_tostring(L, -1);
      log_error(("An exception occured while %s tried to call '%s': %s.\n",
        unitname(mage), fname, error));
      lua_pop(L, 1);
      std::terminate();
    }
  }
  return retval;
}

/** callback for an item-use function written in lua. */
static int
lua_useitem(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
  int retval = 0;
  char fname[64];
  snprintf(fname, sizeof(fname), "use_%s", itype->rtype->_name[0]);

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

/** callback to initialize a familiar from lua. */
static void
lua_initfamiliar(unit * u)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "initfamiliar_%s", u->race->_name[0]);

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      luabind::call_function<int>(L, fname, u);
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

  snprintf(fname, sizeof(fname), "%s_familiar", u->race->_name[0]);
  equip_unit(u, get_equipment(fname));
}

static int
lua_changeresource(unit * u, const struct resource_type * rtype, int delta)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_changeresource", rtype->_name[0]);
  int retval = -1;

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      retval = luabind::call_function<int>(L, fname, u, delta);
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

bool
is_function(struct lua_State * luaState, const char * fname)
{
#if BOOST_VERSION > 103002
  luabind::object globals = luabind::globals(luaState);
  luabind::object fun = globals[fname];
  if (fun.is_valid()) {
    if (luabind::type(fun)==LUA_TFUNCTION) {
      return true;
    }
    log_warning(("Lua global object %s is not a function, type is %u\n", fname, luabind::type(fun)));
  }
#else
  luabind::object globals = luabind::get_globals(luaState);
  luabind::object fun = globals[fname];
  if (fun.is_valid()) {
    if (fun.type()==LUA_TFUNCTION) {
      return true;
    }
    log_warning(("Lua global object %s is not a function, type is %u\n", fname, fun.type()));
  }
#endif
  return false;
}

static int
lua_getresource(unit * u, const struct resource_type * rtype)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_getresource", rtype->_name[0]);
  int retval = -1;

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      retval = luabind::call_function<int>(L, fname, u);
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

void
bind_script(lua_State * L) 
{
  register_function((pf_generic)&lua_callspell, "lua_castspell");
  register_function((pf_generic)&lua_initfamiliar, "lua_initfamiliar");
  register_function((pf_generic)&lua_useitem, "lua_useitem");
  register_function((pf_generic)&lua_getresource, "lua_getresource");
  register_function((pf_generic)&lua_changeresource, "lua_changeresource");
}
