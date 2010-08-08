/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2010   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include "bind_attrib.h"
#include <kernel/config.h>

#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/objtypes.h>
#include <util/attrib.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/storage.h>

#include <bson/bson.h>

#include <lua.h>
#include <tolua.h>
#include <errno.h>

static void 
init_ext(attrib * a) {
  lua_State * L = (lua_State *)global.vm_state;

  lua_pushstring(L, "callbacks");
  lua_rawget(L, LUA_GLOBALSINDEX);
  if (lua_istable(L, -1)) {
    lua_pushstring(L, "attrib_init");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_isfunction(L, -1)) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, a->data.i);
      if (lua_pcall(L, 1, 0, 0)!=0) {
        const char* error = lua_tostring(L, -1);
        log_error(("attrib_init '%d': %s.\n", a->data.i, error));
      }
    }
  }
}

static void 
free_ext(attrib * a) {
  lua_State * L = (lua_State *)global.vm_state;
  if (a->data.i>0) {
    luaL_unref(L, LUA_REGISTRYINDEX, a->data.i);
  }
}

static int
age_ext(attrib * a) {
  return AT_AGE_KEEP;
}

static void
write_ext_i(lua_State * L, const char * name, bson_buffer * bb)
{
  int type = lua_type(L, -1);
  switch (type) {
    case LUA_TNUMBER:
      {
        double value = tolua_tonumber(L, -1, 0);
        bson_append_double(bb, name, value);
      }
      break;
    case LUA_TSTRING:
      {
        const char * value = tolua_tostring(L, -1, 0);
        bson_append_string(bb, name, value);
      }
      break;
    case LUA_TTABLE:
      {
        int n = luaL_getn(L, -1);
        if (n) {
          bson_buffer * arr = bson_append_start_array(bb, name);
          int i;
          for (i=0;i!=n;++i) {
            char num[12];
            bson_numstr(num, i);
            lua_rawgeti(L, -1, i+1);
            write_ext_i(L, num, arr);
            lua_pop(L, 1);
          }
          bson_append_finish_object(arr);
        } else {
          bson_buffer * sub = bson_append_start_object(bb, name);
          lua_pushnil(L);  /* first key */
          while (lua_next(L, -2) != 0) {
            const char * key;
            /* uses 'key' (at index -2) and 'value' (at index -1) */
            lua_pushvalue(L, -2);
            key = lua_tolstring(L, -1, 0);
            lua_pushvalue(L, -2);
            if (key) {
              write_ext_i(L, key, sub);
            }
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 3);
          }
          bson_append_finish_object(sub);
        }
      }
      break;
    case LUA_TUSERDATA:
      {
        tolua_Error tolua_err;
        if (tolua_isusertype(L, -1, "unit", 0, &tolua_err)) {
          unit * u = (unit *)tolua_tousertype(L, -1, 0);
          bson_oid_t oid;
          oid.ints[0] = TYP_UNIT;
          oid.ints[1] = u->no;
          bson_append_oid(bb, name, &oid);
        } else if (tolua_isusertype(L, -1, "region", 0, &tolua_err)) {
          region * r = (region *)tolua_tousertype(L, -1, 0);
          bson_oid_t oid;
          oid.ints[0] = TYP_REGION;
          oid.ints[1] = r->uid;
          bson_append_oid(bb, name, &oid);
        } else if (tolua_isusertype(L, -1, "ship", 0, &tolua_err)) {
          ship * sh = (ship *)tolua_tousertype(L, -1, 0);
          bson_oid_t oid;
          oid.ints[0] = TYP_SHIP;
          oid.ints[1] = sh->no;
          bson_append_oid(bb, name, &oid);
        } else if (tolua_isusertype(L, -1, "building", 0, &tolua_err)) {
          building * b = (building *)tolua_tousertype(L, -1, 0);
          bson_oid_t oid;
          oid.ints[0] = TYP_BUILDING;
          oid.ints[1] = b->no;
          bson_append_oid(bb, name, &oid);
        } else {
          log_error(("unsuported type.\n"));
          bson_append_null(bb, name);
        }
      }
      break;
    default:
      bson_append_null(bb, name);
      break;
  }
}

static void
write_ext(const attrib * a, const void * owner, struct storage * store) {
  lua_State * L = (lua_State *)global.vm_state;
  if (a->data.i>0) {
    int handle = a->data.i;
    bson_buffer bb;
    bson b;
    
    bson_buffer_init( & bb );
    lua_rawgeti(L, LUA_REGISTRYINDEX, handle);
    write_ext_i(L, "_data", &bb);
    bson_from_buffer(&b, &bb);
    store->w_int(store, bson_size(&b));
    store->w_bin(store, b.data, bson_size(&b));
    bson_destroy(&b);
  }
}

static int
read_ext_i(lua_State * L, bson_iterator * it, bson_type type)
{
  switch (type) {
    case bson_double:
      {
        lua_pushnumber(L, bson_iterator_double(it));
      }
      break;
    case bson_string:
      {
        lua_pushstring(L, bson_iterator_string(it));
      }
      break;
    case bson_array:
      {
        bson_iterator sub;
        int err;
        bson_iterator_subiterator(it, &sub);
        lua_newtable(L);
        if (bson_iterator_more(&sub)) {
          bson_type ctype;
          for (ctype = bson_iterator_next(&sub); bson_iterator_more(&sub); ctype = bson_iterator_next(&sub)) {
            int i = atoi(bson_iterator_key(&sub));
            err = read_ext_i(L, &sub, ctype);
            if (err) {
              lua_pop(L, 1);
              return err;
            }
            lua_rawseti(L, -2, i+1);
          }
        }
      }
      break;
    case bson_object:
      {
        bson_iterator sub;
        int err;
        bson_iterator_subiterator(it, &sub);
        lua_newtable(L);
        if (bson_iterator_more(&sub)) {
          bson_type ctype;
          for (ctype = bson_iterator_next(&sub); bson_iterator_more(&sub); ctype = bson_iterator_next(&sub)) {
            lua_pushstring(L, bson_iterator_key(&sub));
            err = read_ext_i(L, &sub, ctype);
            if (err) {
              lua_pop(L, 1);
              return err;
            }
            lua_rawset(L, -3);
          }
        }
      }
      break;
    case bson_oid:
      {
        bson_oid_t * oid = bson_iterator_oid(it);
        if (oid->ints[0]==TYP_UNIT) {
          unit * u = findunit(oid->ints[1]);
          if (u) tolua_pushusertype(L, u, "unit");
          else lua_pushnil(L);
        } else if (oid->ints[0]==TYP_REGION) {
          region * r = findregionbyid(oid->ints[1]);
          if (r) tolua_pushusertype(L, r, "region");
          else lua_pushnil(L);
        } else if (oid->ints[0]==TYP_SHIP) {
          ship * sh = findship(oid->ints[1]);
          if (sh) tolua_pushusertype(L, sh, "ship");
          else lua_pushnil(L);
        } else if (oid->ints[0]==TYP_BUILDING) {
          building * b = findbuilding(oid->ints[1]);
          if (b) tolua_pushusertype(L, b, "building");
          else lua_pushnil(L);
        }
        else {
          log_error(("unknown oid %d %d %d\n", oid->ints[0], oid->ints[3], oid->ints[2]));
          lua_pushnil(L);
        }
      }
      break;
    case bson_null:
      lua_pushnil(L);
      break;
    case bson_eoo:
      return EFAULT;
    default:
      return EINVAL;
  }
  return 0;
}

static int
resolve_bson(variant data, void * address)
{
  lua_State * L = (lua_State *)global.vm_state;
  bson b;
  int err;
  bson_iterator it;
  attrib * a = (attrib*)address;
  char * buffer = data.v;

  bson_init(&b, buffer, 1);
  bson_iterator_init(&it, b.data);
  err = read_ext_i(L, &it, bson_iterator_next(&it));
  a->data.i = luaL_ref(L, LUA_REGISTRYINDEX);
  bson_destroy(&b);
  return err?AT_READ_FAIL:AT_READ_OK;
}

static int
read_ext(attrib * a, void * owner, struct storage * store) {
  variant data;
  int len = store->r_int(store);
  data.v = bson_malloc(len);
  store->r_bin(store, data.v, (size_t)len);
  a->data.v = 0;
  ur_add(data, a, resolve_bson);
  return AT_READ_OK;
};

attrib_type at_lua_ext = {
  "lua", init_ext, free_ext, age_ext, write_ext, read_ext
};

static int
tolua_attrib_create(lua_State* L)
{
  attrib ** ap = NULL;
  tolua_Error tolua_err;

  if (tolua_isusertype(L, 1, TOLUA_CAST "unit", 0, &tolua_err)) {
    unit * u = (unit *)tolua_tousertype(L, 1, 0);
    ap = &u->attribs;
  }
  if (ap) {
    attrib * a = a_new(&at_lua_ext);
    int handle;

    lua_pushvalue(L, 2);
    handle = luaL_ref(L, LUA_REGISTRYINDEX);
    a->data.i = handle;

    a_add(ap, a);
    tolua_pushusertype(L, (void*)a, TOLUA_CAST "attrib");
    return 1;
  }
  return 0;
}

int
tolua_attrib_data(lua_State * L)
{
  attrib * a = (attrib *)tolua_tousertype(L, 1, 0);
  if (a && a->data.i) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, a->data.i);
    return 1;
  }
  return 0;
}

attrib *
tolua_get_lua_ext(struct attrib * alist)
{
  while (alist && alist->type!=&at_lua_ext) alist = alist->next;
  return alist;
}

int
tolua_attriblist_next(lua_State *L)
{
  attrib** attrib_ptr = (attrib **)lua_touserdata(L, lua_upvalueindex(1));
  attrib * a = *attrib_ptr;
  if (a != NULL) {
    tolua_pushusertype(L, (void*)a, TOLUA_CAST "attrib");
    *attrib_ptr = tolua_get_lua_ext(a->next);
    return 1;
  }
  else return 0;  /* no more values to return */
}


void
tolua_attrib_open(lua_State* L)
{
  at_register(&at_lua_ext);

  tolua_usertype(L, TOLUA_CAST "attrib");

  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_cclass(L, TOLUA_CAST "attrib", TOLUA_CAST "attrib", TOLUA_CAST "", NULL);
    tolua_beginmodule(L, TOLUA_CAST "attrib");
    {
      tolua_function(L, TOLUA_CAST "create", &tolua_attrib_create);
      tolua_variable(L, TOLUA_CAST "data", &tolua_attrib_data, NULL);
    }
    tolua_endmodule(L);
  }
  tolua_endmodule(L);
}
