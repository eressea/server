/* 
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include "bind_storage.h"

#include <kernel/save.h>
#include <kernel/version.h>

#include <util/log.h>

#include <storage.h>
#include <stream.h>
#include <filestream.h>
#include <binarystore.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <tolua.h>

static int tolua_storage_create(lua_State * L)
{
    const char *filename = tolua_tostring(L, 1, 0);
    const char *type = tolua_tostring(L, 2, "rb");
    gamedata *data;

    data = gamedata_open(filename, type, RELEASE_VERSION);
    if (data) {
        tolua_pushusertype(L, (void *)data, TOLUA_CAST "storage");
        return 1;
    }
    log_error("could not open %s, mode %s (%s).", filename, type, strerror(errno));
    return 0;
}

static int tolua_storage_read_unit(lua_State * L)
{
    gamedata *data = (gamedata *)tolua_tousertype(L, 1, 0);
    struct unit *u = read_unit(data);
    tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");
    return 1;
}

static int tolua_storage_write_unit(lua_State * L)
{
    gamedata *data = (gamedata *)tolua_tousertype(L, 1, 0);
    struct unit *u = (struct unit *)tolua_tousertype(L, 2, 0);
    write_unit(data, u);
    return 0;
}

static int tolua_storage_read_float(lua_State * L)
{
    gamedata *data = (gamedata *)tolua_tousertype(L, 1, 0);
    float num;
    READ_FLT(data->store, &num);
    lua_pushnumber(L, num);
    return 1;
}

static int tolua_storage_read_int(lua_State * L)
{
    gamedata *data = (gamedata *)tolua_tousertype(L, 1, 0);
    int num;
    READ_INT(data->store, &num);
    lua_pushinteger(L, num);
    return 1;
}

static int tolua_storage_write(lua_State * L)
{
    gamedata *data = (gamedata *)tolua_tousertype(L, 1, 0);
    if (data->version && tolua_isnumber(L, 2, 0, 0)) {
        lua_Number num = tolua_tonumber(L, 2, 0);
        double n;
        if (modf(num, &n) == 0.0) {
            WRITE_INT(data->store, (int)num);
        }
        else {
            WRITE_FLT(data->store, (float)num);
        }
    }
    return 0;
}

static int tolua_storage_tostring(lua_State * L)
{
    gamedata *data = (gamedata *)tolua_tousertype(L, 1, 0);
    char name[64];
    _snprintf(name, sizeof(name), "<gamedata %p ver=%d>", (void *)data, data->version);
    lua_pushstring(L, name);
    return 1;
}

static int tolua_storage_close(lua_State * L)
{
    gamedata *data = (gamedata *)tolua_tousertype(L, 1, 0);
    gamedata_close(data);
    return 0;
}

void tolua_storage_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "storage");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_cclass(L, TOLUA_CAST "storage", TOLUA_CAST "storage", TOLUA_CAST "",
            NULL);
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
