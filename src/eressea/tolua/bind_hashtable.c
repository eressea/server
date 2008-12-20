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
#include "bind_hashtable.h"

#include <kernel/building.h>
#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/region.h>

#include <attributes/object.h>

#include <util/variant.h>
#include <util/attrib.h>

#include <lua.h>
#include <tolua.h>

#include <assert.h>

static int
tolua_hashtable_get(lua_State* tolua_S)
{
  hashtable self = (hashtable) tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  attrib * a = a_find(*self, &at_object);

  for (; a && a->type == &at_object; a = a->next) {
    const char * obj_name = object_name(a);
    if (obj_name && name && strcmp(obj_name, name) == 0) {
      variant val;
      object_type type;

      object_get(a, &type, &val);
      switch (type) {
        case TNONE:
          lua_pushnil(tolua_S);
          break;
        case TINTEGER:
          lua_pushnumber(tolua_S, (lua_Number)val.i);
          break;
        case TREAL:
          lua_pushnumber(tolua_S, (lua_Number)val.f);
          break;
        case TREGION:
          tolua_pushusertype(tolua_S, val.v, "region");
          break;
        case TBUILDING:
          tolua_pushusertype(tolua_S, val.v, "building");
          break;
        case TUNIT:
          tolua_pushusertype(tolua_S, val.v, "unit");
          break;
        case TSHIP:
          tolua_pushusertype(tolua_S, val.v, "ship");
          break;
        case TSTRING:
          tolua_pushstring(tolua_S, (const char*) val.v);
          break;
        default:
          assert(!"not implemented");
      }
      return 1;
    }
  }
  lua_pushnil(tolua_S);
  return 1;
}

static int
tolua_hashtable_set_number(lua_State* tolua_S)
{
  hashtable self = (hashtable) tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  lua_Number value = tolua_tonumber(tolua_S, 3, 0);
  attrib * a = a_find(*self, &at_object);
  variant val;
  
  val.f = (float)value;

  for (; a && a->type == &at_object; a = a->next) {
    if (strcmp(object_name(a), name) == 0) {
      object_set(a, TREAL, val);
      return 0;
    }
  }

  a = a_add(self, object_create(name, TREAL, val));
  return 0;
}

static int
tolua_hashtable_set_string(lua_State* tolua_S)
{
  hashtable self = (hashtable) tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  const char * value = tolua_tostring(tolua_S, 3, 0);
  attrib * a = a_find(*self, &at_object);
  variant val;

  val.v = strdup(value);

  for (; a && a->type == &at_object; a = a->next) {
    if (strcmp(object_name(a), name) == 0) {
      object_set(a, TSTRING, val);
      return 0;
    }
  }

  a = a_add(self, object_create(name, TSTRING, val));
  return 0;
}

static int
tolua_hashtable_set_usertype(lua_State* tolua_S, int type)
{
  hashtable self = (hashtable) tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  unit * value = tolua_tousertype(tolua_S, 3, 0);
  attrib * a = a_find(*self, &at_object);
  variant val;

  val.v = value;

  for (; a && a->type == &at_object; a = a->next) {
    if (strcmp(object_name(a), name) == 0) {
      object_set(a, type, val);
      return 0;
    }
  }

  a = a_add(self, object_create(name, type, val));
  return 0;
}


static int
tolua_hashtable_set(lua_State* tolua_S)
{
  tolua_Error tolua_err;
  if (tolua_isnumber(tolua_S, 3, 0, &tolua_err)) {
    return tolua_hashtable_set_number(tolua_S);
  } else if (tolua_isusertype(tolua_S, 3, "unit", 0, &tolua_err)) {
    return tolua_hashtable_set_usertype(tolua_S, TUNIT);
  } else if (tolua_isusertype(tolua_S, 3, "faction", 0, &tolua_err)) {
    return tolua_hashtable_set_usertype(tolua_S, TFACTION);
  } else if (tolua_isusertype(tolua_S, 3, "ship", 0, &tolua_err)) {
    return tolua_hashtable_set_usertype(tolua_S, TSHIP);
  } else if (tolua_isusertype(tolua_S, 3, "building", 0, &tolua_err)) {
    return tolua_hashtable_set_usertype(tolua_S, TBUILDING);
  } else if (tolua_isusertype(tolua_S, 3, "region", 0, &tolua_err)) {
    return tolua_hashtable_set_usertype(tolua_S, TREGION);
  }
  return tolua_hashtable_set_string(tolua_S);
}



void
tolua_hashtable_open(lua_State* tolua_S)
{
  /* register user types */
  tolua_usertype(tolua_S, "hashtable");

  tolua_module(tolua_S, NULL, 0);
  tolua_beginmodule(tolua_S, NULL);
  {
    tolua_cclass(tolua_S, "hashtable", "hashtable", "", NULL);
    tolua_beginmodule(tolua_S, "hashtable");
    {
      tolua_function(tolua_S, "get", tolua_hashtable_get);
      tolua_function(tolua_S, "set", tolua_hashtable_set);
    }
    tolua_endmodule(tolua_S);
  }
  tolua_endmodule(tolua_S);
}
