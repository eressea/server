/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include "helpers.h"
#include <config.h>

#include <util/functions.h>
#include <util/log.h>
#include <util/attrib.h>

#include <kernel/eressea.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/magic.h>
#include <kernel/race.h>
#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <gamecode/archetype.h>

#include <lua.h>
#include <tolua.h>

#include <assert.h>

static int
lua_giveitem(unit * s, unit * d, const item_type * itype, int n, struct order * ord)
{
  lua_State * L = (lua_State *)global.vm_state;
  char fname[64];
  int result = -1;
  const char * iname = itype->rtype->_name[0];

  assert(s!=NULL);
  strcat(strcpy(fname, iname), "_give");

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, s, "unit");
    tolua_pushusertype(L, d, "unit");
    tolua_pushstring(L, iname);
    tolua_pushnumber(L, (lua_Number)n);

    if (lua_pcall(L, 4, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("unit %s calling '%s': %s.\n",
        unitname(s), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("unit %s trying to call '%s' : not a function.\n",
      unitname(s), fname));
    lua_pop(L, 1);
  }

  return result;
}

static int
limit_resource(const region * r, const resource_type * rtype)
{
  char fname[64];
  int result = -1;
  lua_State * L = (lua_State *)global.vm_state;

  snprintf(fname, sizeof(fname), "%s_limit", rtype->_name[0]);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)r, "region");

    if (lua_pcall(L, 1, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("limit(%s) calling '%s': %s.\n",
        regionname(r, NULL), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("limit(%s) calling '%s': not a function.\n",
      regionname(r, NULL), fname));
    lua_pop(L, 1);
  }

  return result;
}

static void
produce_resource(region * r, const resource_type * rtype, int norders)
{
  lua_State * L = (lua_State *)global.vm_state;
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_produce", rtype->_name[0]);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)r, "region");
    tolua_pushnumber(L, (lua_Number)norders);

    if (lua_pcall(L, 2, 0, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("produce(%s) calling '%s': %s.\n",
        regionname(r, NULL), fname, error));
      lua_pop(L, 1);
    }
  } else {
    log_error(("produce(%s) calling '%s': not a function.\n",
      regionname(r, NULL), fname));
    lua_pop(L, 1);
  }
}


static int
lc_age(struct attrib * a)
{
  building_action * data = (building_action*)a->data.v;
  const char * fname = data->fname;
  const char * fparam = data->param;
  building * b = data->b;
  int result = -1;

  assert(b!=NULL);
  if (fname!=NULL) {
    lua_State * L = (lua_State *)global.vm_state;

    lua_pushstring(L, fname);
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_isfunction(L, 1)) {
      tolua_pushusertype(L, (void *)b, "building");
      if (fparam) {
        tolua_pushstring(L, fparam);
      }

      if (lua_pcall(L, fparam?2:1, 1, 0)!=0) {
        const char* error = lua_tostring(L, -1);
        log_error(("lc_age(%s) calling '%s': %s.\n",
          buildingname(b), fname, error));
        lua_pop(L, 1);
      } else {
        result = (int)lua_tonumber(L, -1);
        lua_pop(L, 1);
      }
    } else {
      log_error(("lc_age(%s) calling '%s': not a function.\n",
        buildingname(b), fname));
      lua_pop(L, 1);
    }
  }
  return (result!=0)?AT_AGE_KEEP:AT_AGE_REMOVE;
}


/** callback to use lua for spell functions */
static int
lua_callspell(castorder *co)
{
  lua_State * L = (lua_State *)global.vm_state;
  const char * fname = co->sp->sname;
  unit * mage = co->familiar?co->familiar:co->magician.u;
  int result = -1;
  const char * hashpos = strchr(fname, '#');
  char fbuf[64];

  if (hashpos!=NULL) {
    ptrdiff_t len = hashpos - fname;
    assert(len<(ptrdiff_t)sizeof(fbuf));
    strncpy(fbuf, fname, len);
    fbuf[len] = '\0';
    fname = fbuf;
  }

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, co->rt, "region");
    tolua_pushusertype(L, mage, "unit");
    tolua_pushnumber(L, (lua_Number)co->level);
    tolua_pushnumber(L, (lua_Number)co->force);

    if (lua_pcall(L, 4, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("spell(%s) calling '%s': %s.\n",
        unitname(mage), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("spell(%s) calling '%s': not a function.\n",
      unitname(mage), fname));
    lua_pop(L, 1);
  }

  return result;
}

/** callback to initialize a familiar from lua. */
static void
lua_initfamiliar(unit * u)
{
  lua_State * L = (lua_State *)global.vm_state;
  char fname[64];
  int result = -1;
  snprintf(fname, sizeof(fname), "initfamiliar_%s", u->race->_name[0]);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, u, "unit");

    if (lua_pcall(L, 1, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("familiar(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_warning(("familiar(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }

  create_mage(u, M_GRAY);

  snprintf(fname, sizeof(fname), "%s_familiar", u->race->_name[0]);
  equip_unit(u, get_equipment(fname));
}

static int
lua_changeresource(unit * u, const struct resource_type * rtype, int delta)
{
  lua_State * L = (lua_State *)global.vm_state;
  int result = -1;
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_changeresource", rtype->_name[0]);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, u, "unit");
    tolua_pushnumber(L, (lua_Number)delta);

    if (lua_pcall(L, 2, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("change(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("change(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }

  return result;
}

static int
lua_getresource(unit * u, const struct resource_type * rtype)
{
  lua_State * L = (lua_State *)global.vm_state;
  int result = -1;
  char fname[64];
  snprintf(fname, sizeof(fname), "%s_getresource", rtype->_name[0]);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, u, "unit");

    if (lua_pcall(L, 1, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("get(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("get(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }

  return result;
}

static int
lua_canuse_item(const unit * u, const struct item_type * itype)
{
  lua_State * L = (lua_State *)global.vm_state;
  int result = -1;
  const char * fname = "item_canuse";

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)u, "unit");
    tolua_pushstring(L, itype->rtype->_name[0]);

    if (lua_pcall(L, 2, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("get(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = lua_toboolean(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("get(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }

  return result;
}

static int
lua_wage(const region * r, const faction * f, const race * rc)
{
  lua_State * L = (lua_State *)global.vm_state;
  const char * fname = "wage";
  int result = -1;

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)r, "region");
    tolua_pushusertype(L, (void *)f, "faction");
    tolua_pushstring(L, rc?rc->_name[0]:0);

    if (lua_pcall(L, 3, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("wage(%s) calling '%s': %s.\n",
        regionname(r, NULL), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("wage(%s) calling '%s': not a function.\n",
      regionname(r, NULL), fname));
    lua_pop(L, 1);
  }

  return result;
}

static void
lua_agebuilding(building * b)
{
  lua_State * L = (lua_State *)global.vm_state;
  char fname[64];

  snprintf(fname, sizeof(fname), "age_%s", b->type->_name);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)b, "building");

    if (lua_pcall(L, 1, 0, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("agebuilding(%s) calling '%s': %s.\n",
        buildingname(b), fname, error));
      lua_pop(L, 1);
    }
  } else {
    log_error(("agebuilding(%s) calling '%s': not a function.\n",
      buildingname(b), fname));
    lua_pop(L, 1);
  }
}

static int
lua_maintenance(const unit * u)
{
  lua_State * L = (lua_State *)global.vm_state;
  const char * fname = "maintenance";
  int result = -1;

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)u, "unit");

    if (lua_pcall(L, 1, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("maintenance(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("maintenance(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }

  return result;
}

static void
lua_equipmentcallback(const struct equipment * eq, unit * u)
{
  lua_State * L = (lua_State *)global.vm_state;
  char fname[64];
  int result = -1;
  snprintf(fname, sizeof(fname), "equip_%s", eq->name);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)u, "unit");

    if (lua_pcall(L, 1, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("equip(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("equip(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }
}

/** callback for an item-use function written in lua. */
int
lua_useitem(struct unit * u, const struct item_type * itype, int amount, struct order * ord)
{
  lua_State * L = (lua_State *)global.vm_state;
  int result = 0;
  char fname[64];
  snprintf(fname, sizeof(fname), "use_%s", itype->rtype->_name[0]);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)u, "unit");
    tolua_pushnumber(L, (lua_Number)amount);

    if (lua_pcall(L, 2, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("use(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("use(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }

  return result;
}

static int
lua_recruit(struct unit * u, const struct archetype * arch, int amount)
{
  lua_State * L = (lua_State *)global.vm_state;
  int result = 0;
  char fname[64];
  snprintf(fname, sizeof(fname), "recruit_%s", arch->name[0]);

  lua_pushstring(L, fname);
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_isfunction(L, 1)) {
    tolua_pushusertype(L, (void *)u, "unit");
    tolua_pushnumber(L, (lua_Number)amount);

    if (lua_pcall(L, 2, 1, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("use(%s) calling '%s': %s.\n",
        unitname(u), fname, error));
      lua_pop(L, 1);
    } else {
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);
    }
  } else {
    log_error(("use(%s) calling '%s': not a function.\n",
      unitname(u), fname));
    lua_pop(L, 1);
  }
  return result;
}

void
register_tolua_helpers(void)
{
  at_building_action.age = lc_age;

  register_function((pf_generic)&lua_agebuilding, "lua_agebuilding");
  register_function((pf_generic)&lua_recruit, "lua_recruit");
  register_function((pf_generic)&lua_callspell, "lua_castspell");
  register_function((pf_generic)&lua_initfamiliar, "lua_initfamiliar");
  register_item_use(&lua_useitem, "lua_useitem");
  register_function((pf_generic)&lua_getresource, "lua_getresource");
  register_function((pf_generic)&lua_canuse_item, "lua_canuse_item");
  register_function((pf_generic)&lua_changeresource, "lua_changeresource");
  register_function((pf_generic)&lua_equipmentcallback, "lua_equip");

  register_function((pf_generic)&lua_wage, "lua_wage");
  register_function((pf_generic)&lua_maintenance, "lua_maintenance");


  register_function((pf_generic)produce_resource, "lua_produceresource");
  register_function((pf_generic)limit_resource, "lua_limitresource");
  register_item_give(lua_giveitem, "lua_giveitem");
}
