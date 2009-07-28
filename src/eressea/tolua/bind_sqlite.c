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

#include "bind_unit.h"
#include "bindings.h"

#include <sqlite3.h>
#include <lua.h>
#include <tolua.h>

#define LTYPE_DB TOLUA_CAST "db"

extern int db_update_factions(sqlite3 * db);
static int
tolua_db_update_factions(lua_State* L)
{  
  sqlite3 * db = (sqlite3 *)tolua_tousertype(L, 1, 0);
  db_update_factions(db);
  return 0;
}

static int
tolua_db_close(lua_State* L)
{  
  sqlite3 * db = (sqlite3 *)tolua_tousertype(L, 1, 0);
  sqlite3_close(db);
  return 0;
}

static int
tolua_db_create(lua_State* L)
{  
  sqlite3 * db;
  const char * dbname = tolua_tostring(L, 1, 0);
  int result = sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READWRITE, 0);
  if (result==SQLITE_OK) {
    tolua_pushusertype(L, (void*)db, LTYPE_DB);
    return 1;
  }
  return 0;
}

int
tolua_sqlite_open(lua_State * L)
{
  /* register user types */

  tolua_usertype(L, LTYPE_DB);

  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_cclass(L, LTYPE_DB, LTYPE_DB, TOLUA_CAST "", NULL);
    tolua_beginmodule(L, LTYPE_DB);
    {
      tolua_function(L, TOLUA_CAST "open", &tolua_db_create);
      tolua_function(L, TOLUA_CAST "close", &tolua_db_close);

      tolua_function(L, TOLUA_CAST "update_factions", &tolua_db_update_factions);
    }
    tolua_endmodule(L);

  }
  tolua_endmodule(L);
  return 0;
}
