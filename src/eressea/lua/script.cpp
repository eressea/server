/* vi: set ts=2:
 +-------------------+
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea.de>
 | (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <boost/version.hpp>
#include <lua.hpp>
#include <kernel/eressea.h>
#include "script.h"
#include "bindings.h"

// kernel includes
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

// util includes
#include <util/attrib.h>
#include <util/functions.h>
#include <util/log.h>

#pragma warning (push)
#pragma warning (disable: 4127)
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>
#pragma warning (pop)

#include <cstdio>
#include <cstring>

using namespace luabind;

static void
free_script(attrib * a) {
  if (a->data.v!=NULL) {
    object * f = (object *)a->data.v;
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
    object * func = (object *)a->data.v;
    try {
      func->operator()(u);
    }
    catch (error& e) {
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
    object * f = (object *)a->data.v;
    delete f;
  }
  a->data.v = fptr;
}

/** callback to use lua for spell functions */
static int
lua_callspell(castorder *co)
{
  const char * fname = co->sp->sname;
  unit * mage = co->familiar?co->familiar:co->magician.u;
  int retval = -1;
  const char * hashpos = strchr(fname, '#');
  char fbuf[64];

  if (hashpos!=NULL) {
    ptrdiff_t len = hashpos - fname;
    assert(len<sizeof(fbuf));
    strncpy(fbuf, fname, len);
    fbuf[len] = '\0';
    fname = fbuf;
  }

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      retval = call_function<int>(L, fname, co->rt, mage, co->level, co->force);
    }
    catch (error& e) {
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
      retval = call_function<int>(L, fname, u, amount);
    }
    catch (error& e) {
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
      call_function<int>(L, fname, u);
    }
    catch (error& e) {
      lua_State* L = e.state();
      const char* error = lua_tostring(L, -1);
      log_error(("An exception occured while %s tried to call '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
      std::terminate();
    }
  }

  snprintf(fname, sizeof(fname), "%s_familiar", u->race->_name[0]);
  create_mage(u, M_GRAU);
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
      retval = call_function<int>(L, fname, u, delta);
    }
    catch (error& e) {
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

static int
lua_getresource(unit * u, const struct resource_type * rtype)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_getresource", rtype->_name[0]);
  int retval = -1;

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      retval = call_function<int>(L, fname, u);
    }
    catch (error& e) {
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

struct script_interface {
  void destroy() {
    delete wage;
    wage = 0;
    delete maintenance;
    maintenance = 0;
  }
  object * wage;
  object * maintenance;
};

static script_interface interface;

static int
lua_wage(const region * r, const faction * f, const race * rc)
{
  int retval = -1;

  assert(interface.wage);
  try {
    object o = interface.wage->operator()(r, f, rc);
    retval = object_cast<int>(o);
  }
  catch (error& e) {
    lua_State* L = e.state();
    const char* error = lua_tostring(L, -1);
    log_error(("An exception occured in interface 'wage': %s.\n", error));
    lua_pop(L, 1);
    std::terminate();
  }
  return retval;
}

static int
lua_maintenance(const unit * u)
{
  int retval = -1;
  assert(interface.maintenance);

  try {
    object o = interface.maintenance->operator()(u);
    retval = object_cast<int>(o);
  }
  catch (error& e) {
    lua_State* L = e.state();
    const char* error = lua_tostring(L, -1);
    log_error(("An exception occured in interface 'maintenance': %s.\n", error));
    lua_pop(L, 1);
    std::terminate();
  }
  return retval;
}

static void
overload(const char * name, const object& f)
{
  if (strcmp(name, "wage")==0) {
    global.functions.wage = &lua_wage;
    interface.wage = new object(f);
  } else if (strcmp(name, "maintenance")==0) {
    global.functions.maintenance = &lua_maintenance;
    interface.maintenance = new object(f);;
  }
}

static void
unit_setscript(struct unit& u, const luabind::object& f)
{
  luabind::object * fptr = new luabind::object(f);
  setscript(&u.attribs, fptr);
}

static void
race_setscript(const char * rcname, const luabind::object& f)
{
  race * rc = rc_find(rcname);
  if (rc!=NULL) {
    luabind::object * fptr = new luabind::object(f);
    setscript(&rc->attribs, fptr);
  }
}

static void
lua_equipmentcallback(const struct equipment * eq, unit * u)
{
  char fname[64];
  snprintf(fname, sizeof(fname), "equip_%s", eq->name);

  lua_State * L = (lua_State *)global.vm_state;
  if (is_function(L, fname)) {
    try {
      call_function<int>(L, fname, u);
    }
    catch (error& e) {
      lua_State* L = e.state();
      const char* error = lua_tostring(L, -1);
      log_error(("An exception occured while %s tried to call '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
      std::terminate();
    }
  }
}

void
bind_script(lua_State * L)
{
  register_function((pf_generic)&lua_callspell, "lua_castspell");
  register_function((pf_generic)&lua_initfamiliar, "lua_initfamiliar");
  register_item_use(&lua_useitem, "lua_useitem");
  register_function((pf_generic)&lua_getresource, "lua_getresource");
  register_function((pf_generic)&lua_changeresource, "lua_changeresource");
  register_function((pf_generic)&lua_equipmentcallback, "lua_equip");

  module(L)[
    def("overload", &overload),
    def("set_race_brain", &race_setscript),
    def("set_unit_brain", &unit_setscript)
  ];
}

void
reset_scripts()
{
  interface.destroy();
}
