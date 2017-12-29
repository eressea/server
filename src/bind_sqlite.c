#ifdef _MSC_VER
#include <platform.h>
#endif

#include "bind_unit.h"
#include "bindings.h"

#include <kernel/config.h> /* for game_id */
#include <util/macros.h>

#include <sqlite3.h>
#include <tolua.h>
#include <stdbool.h>

#define LTYPE_DB TOLUA_CAST "db"

extern int db_update_factions(sqlite3 * db, bool force, int game);
static int tolua_db_update_factions(lua_State * L)
{
    sqlite3 *db = (sqlite3 *)tolua_tousertype(L, 1, 0);
    db_update_factions(db, tolua_toboolean(L, 2, 0), game_id());
    return 0;
}

extern int db_update_scores(sqlite3 * db, bool force);
static int tolua_db_update_scores(lua_State * L)
{
    sqlite3 *db = (sqlite3 *)tolua_tousertype(L, 1, 0);
    db_update_scores(db, tolua_toboolean(L, 2, 0));
    return 0;
}

static int tolua_db_execute(lua_State * L)
{
    sqlite3 *db = (sqlite3 *)tolua_tousertype(L, 1, 0);
    const char *sql = tolua_tostring(L, 2, 0);

    int res = sqlite3_exec(db, sql, 0, 0, 0);

    lua_pushinteger(L, res);
    return 1;
}

static int tolua_db_close(lua_State * L)
{
    sqlite3 *db = (sqlite3 *)tolua_tousertype(L, 1, 0);
    sqlite3_close(db);
    return 0;
}

static int tolua_db_create(lua_State * L)
{
    sqlite3 *db;
    const char *dbname = tolua_tostring(L, 1, 0);
    int result = sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READWRITE, 0);
    if (result == SQLITE_OK) {
        tolua_pushusertype(L, (void *)db, LTYPE_DB);
        return 1;
    }
    return 0;
}

int tolua_sqlite_open(lua_State * L)
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

            tolua_function(L, TOLUA_CAST "update_factions",
                &tolua_db_update_factions);
            tolua_function(L, TOLUA_CAST "update_scores", &tolua_db_update_scores);
            tolua_function(L, TOLUA_CAST "execute", &tolua_db_execute);
        }
        tolua_endmodule(L);

    }
    tolua_endmodule(L);
    return 0;
}
