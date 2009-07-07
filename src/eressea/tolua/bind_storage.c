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

#include <config.h>
#include <kernel/types.h>
#include "bind_storage.h"

#include <util/storage.h>
#include <kernel/save.h>
#include <kernel/textstore.h>
#include <kernel/binarystore.h>

#include <math.h>

#include <lua.h>
#include <tolua.h>


static int
tolua_storage_create(lua_State* L)
{
  const char * filename = tolua_tostring(L, 1, 0);
  const char * type = tolua_tostring(L, 2, "rb");
  storage * store = 0;
  int mode = IO_READ;
  if (strchr(type, 't')) {
    store = malloc(sizeof(text_store));
    memcpy(store, &text_store, sizeof(text_store));
  } else {
    store = malloc(sizeof(binary_store));
    memcpy(store, &binary_store, sizeof(binary_store));
  }
  if (strchr(type, 'r')) mode = IO_READ;
  if (strchr(type, 'w')) mode = IO_WRITE;
  store->open(store, filename, mode);
  tolua_pushusertype(L, (void*)store, TOLUA_CAST "storage");
  return 1;
}

static int
tolua_storage_read_unit(lua_State *L)
{
  storage * self = (storage *)tolua_tousertype(L, 1, 0);
  struct unit * u = read_unit(self);
  tolua_pushusertype(L, (void*)u, TOLUA_CAST "unit");
  return 1;
}

static int
tolua_storage_write_unit(lua_State *L)
{
  storage * self = (storage *)tolua_tousertype(L, 1, 0);
  struct unit * u = (struct unit *)tolua_tousertype(L, 2, 0);
  write_unit(self, u);
  return 0;
}


static int
tolua_storage_read_float(lua_State *L)
{
  storage * self = (storage *)tolua_tousertype(L, 1, 0);
  float num = self->r_flt(self);
  tolua_pushnumber(L, (lua_Number)num);
  return 1;
}

static int
tolua_storage_read_int(lua_State *L)
{
  storage * self = (storage *)tolua_tousertype(L, 1, 0);
  int num = self->r_int(self);
  tolua_pushnumber(L, (lua_Number)num);
  return 1;
}

static int
tolua_storage_write(lua_State *L)
{
  storage * self = (storage *)tolua_tousertype(L, 1, 0);
  if (tolua_isnumber(L, 2, 0, 0)) {
    lua_Number num = tolua_tonumber(L, 2, 0);
    double n;
    if (modf(num, &n)==0.0) {
      self->w_int(self, (int)num);
    } else {
      self->w_flt(self, (float)num);
    }
  }
  return 0;
}

static int
tolua_storage_tostring(lua_State *L)
{
  storage * self = (storage *)tolua_tousertype(L, 1, 0);
  char name[64];
  snprintf(name, sizeof(name), "<storage enc=%d ver=%d>", self->encoding, self->version);
  lua_pushstring(L, name);
  return 1;
}

static int
tolua_storage_close(lua_State *L)
{
  storage * self = (storage *)tolua_tousertype(L, 1, 0);
  self->close(self);
  return 0;
}

void
tolua_storage_open(lua_State* L)
{
  /* register user types */
  tolua_usertype(L, TOLUA_CAST "storage");

  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_cclass(L, TOLUA_CAST "storage", TOLUA_CAST "storage", TOLUA_CAST "", NULL);
    tolua_beginmodule(L, TOLUA_CAST "storage");
    {
      tolua_function(L, TOLUA_CAST "__tostring", tolua_storage_tostring);
      tolua_function(L, TOLUA_CAST "write", tolua_storage_write);
      tolua_function(L, TOLUA_CAST "read_int", tolua_storage_read_int);
      tolua_function(L, TOLUA_CAST "read_float", tolua_storage_read_float);
      tolua_function(L, TOLUA_CAST "write_unit", tolua_storage_write_unit);
      tolua_function(L, TOLUA_CAST "read_unit", tolua_storage_read_unit);
      tolua_function(L, TOLUA_CAST "close", tolua_storage_close);
      tolua_function(L, TOLUA_CAST "create", tolua_storage_create);
    }
    tolua_endmodule(L);
  }
  tolua_endmodule(L);
}
