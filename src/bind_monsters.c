#ifdef _MSC_VER
#include <platform.h>
#endif

#include "monsters.h"

#include <spells/flyingship.h>

#include <kernel/faction.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <util/macros.h>

#include <lua.h>
#include <tolua.h>

#include <stdlib.h>

static int tolua_levitate_ship(lua_State * L)
{
    ship *sh = (ship *)tolua_tousertype(L, 1, 0);
    unit *mage = (unit *)tolua_tousertype(L, 2, 0);
    float power = (float)tolua_tonumber(L, 3, 0);
    int duration = (int)tolua_tonumber(L, 4, 0);
    int cno = levitate_ship(sh, mage, power, duration);
    lua_pushinteger(L, cno);
    return 1;
}

static int tolua_planmonsters(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, get_monsters());
    if (f) {
        plan_monsters(f);
    }

    return 0;
}

static int tolua_spawn_dragons(lua_State * L)
{
    UNUSED_ARG(L);
    spawn_dragons();
    return 0;
}

static int tolua_get_monsters(lua_State * L)
{
    tolua_pushusertype(L, get_monsters(), "faction");
    return 1;
}

static int tolua_spawn_undead(lua_State * L)
{
    UNUSED_ARG(L);
    spawn_undead();
    return 0;
}

void bind_monsters(lua_State *L)
{
    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_function(L, TOLUA_CAST "levitate_ship", tolua_levitate_ship);
        tolua_function(L, TOLUA_CAST "plan_monsters", tolua_planmonsters);
        tolua_function(L, TOLUA_CAST "spawn_undead", tolua_spawn_undead);
        tolua_function(L, TOLUA_CAST "spawn_dragons", tolua_spawn_dragons);
        tolua_function(L, TOLUA_CAST "get_monsters", tolua_get_monsters);
    }
    tolua_endmodule(L);
}
