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
#include "helpers.h"
#include "vortex.h"
#include "alchemy.h"

#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/resolve.h>

#include <kernel/callbacks.h>
#include <kernel/config.h>
#include <kernel/callbacks.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/spell.h>
#include <kernel/race.h>
#include <kernel/resources.h>
#include <kernel/unit.h>
#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <storage.h>

#include <tolua.h>
#include <lua.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int
lua_giveitem(unit * s, unit * d, const item_type * itype, int n, struct order *ord)
{
    lua_State *L = (lua_State *)global.vm_state;
    char fname[64];
    int result = -1;
    const char *iname = itype->rtype->_name;

    assert(s != NULL);
    strlcpy(fname, iname, sizeof(fname));
    strlcat(fname, "_give", sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, s, TOLUA_CAST "unit");
        tolua_pushusertype(L, d, TOLUA_CAST "unit");
        tolua_pushstring(L, iname);
        lua_pushinteger(L, n);

        if (lua_pcall(L, 4, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("unit %s calling '%s': %s.\n", unitname(s), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("unit %s trying to call '%s' : not a function.\n", unitname(s), fname);
        lua_pop(L, 1);
    }

    return result;
}

static int limit_resource_lua(const region * r, const resource_type * rtype)
{
    char fname[64];
    int result = -1;
    lua_State *L = (lua_State *)global.vm_state;

    strlcpy(fname, rtype->_name, sizeof(fname));
    strlcat(fname, "_limit", sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)r, TOLUA_CAST "region");

        if (lua_pcall(L, 1, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("limit(%s) calling '%s': %s.\n", regionname(r, NULL), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("limit(%s) calling '%s': not a function.\n", regionname(r, NULL), fname);
        lua_pop(L, 1);
    }

    return result;
}

static void
produce_resource_lua(region * r, const resource_type * rtype, int norders)
{
    lua_State *L = (lua_State *)global.vm_state;
    char fname[64];

    strlcpy(fname, rtype->_name, sizeof(fname));
    strlcat(fname, "_produce", sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)r, TOLUA_CAST "region");
        lua_pushinteger(L, norders);

        if (lua_pcall(L, 2, 0, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("produce(%s) calling '%s': %s.\n", regionname(r, NULL), fname, error);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("produce(%s) calling '%s': not a function.\n", regionname(r, NULL), fname);
        lua_pop(L, 1);
    }
}

static void push_param(lua_State * L, char c, spllprm * param)
{
    if (c == 'u')
        tolua_pushusertype(L, param->data.u, "unit");
    else if (c == 'b')
        tolua_pushusertype(L, param->data.b, "building");
    else if (c == 's')
        tolua_pushusertype(L, param->data.sh, "ship");
    else if (c == 'r')
        tolua_pushusertype(L, param->data.sh, "region");
    else if (c == 'c')
        tolua_pushstring(L, param->data.s);
    else {
        log_error("unsupported syntax %c.\n", c);
        lua_pushnil(L);
    }
}

/** callback to use lua for spell functions */
static int lua_callspell(castorder * co, const char *fname)
{
    lua_State *L = (lua_State *)global.vm_state;
    unit *caster = co_get_caster(co);
    region * r = co_get_region(co);
    int result = -1;

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        int nparam = 4;
        tolua_pushusertype(L, r, TOLUA_CAST "region");
        tolua_pushusertype(L, caster, TOLUA_CAST "unit");
        lua_pushinteger(L, co->level);
        lua_pushnumber(L, co->force);
        if (co->sp->parameter && co->par->length) {
            const char *synp = co->sp->parameter;
            int i = 0;
            ++nparam;
            lua_newtable(L);
            while (*synp && i < co->par->length) {
                spllprm *param = co->par->param[i];
                char c = *synp;
                if (c == '+') {
                    push_param(L, *(synp - 1), param);
                }
                else {
                    push_param(L, c, param);
                    ++synp;
                }
                lua_rawseti(L, -2, ++i);
            }
        }

        if (lua_pcall(L, nparam, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("spell(%s) calling '%s': %s.\n", unitname(caster), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        int ltype = lua_type(L, -1);
        log_error("spell(%s) calling '%s': not a function, has type %d.\n", unitname(caster), fname, ltype);
        lua_pop(L, 1);
    }

    return result;
}

static int
lua_changeresource(unit * u, const struct resource_type *rtype, int delta)
{
    lua_State *L = (lua_State *)global.vm_state;
    int result = -1;
    char fname[64];

    strlcpy(fname, rtype->_name, sizeof(fname));
    strlcat(fname, "_changeresource", sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, u, TOLUA_CAST "unit");
        lua_pushinteger(L, delta);

        if (lua_pcall(L, 2, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("change(%s) calling '%s': %s.\n", unitname(u), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("change(%s) calling '%s': not a function.\n", unitname(u), fname);
        lua_pop(L, 1);
    }

    return result;
}

/** callback for an item-use function written in lua. */
static int
use_item_lua(unit *u, const item_type *itype, int amount, struct order *ord)
{
    lua_State *L = (lua_State *)global.vm_state;
    int result = 0;
    char fname[64];
    int (*callout)(unit *, const item_type *, int, struct order *);

    strlcpy(fname, "use_", sizeof(fname));
    strlcat(fname, itype->rtype->_name, sizeof(fname));

    callout = (int(*)(unit *, const item_type *, int, struct order *))get_function(fname);
    if (callout) {
        return callout(u, itype, amount, ord);
    }

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");
        lua_pushinteger(L, amount);
        lua_pushstring(L, getstrtoken());
        tolua_pushusertype(L, (void *)ord, TOLUA_CAST "order");
        if (lua_pcall(L, 4, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("use(%s) calling '%s': %s.\n", unitname(u), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
        return result;
    }
    lua_pop(L, 1);
    if (itype->rtype->ptype) {
        return use_potion(u, itype, amount, ord);
    } else {
        log_error("no such callout: %s", fname);
    }
    log_error("use(%s) calling '%s': not a function.\n", unitname(u), fname);

    return result;
}

/* compat code for old data files */
static int caldera_read(trigger *t, struct gamedata *data)
{
    UNUSED_ARG(t);
    read_building_reference(data);
    return AT_READ_FAIL;
}

struct trigger_type tt_caldera = {
    "caldera",
    NULL, NULL, NULL, NULL,
    caldera_read
};

void register_tolua_helpers(void)
{
    tt_register(&tt_caldera);
    at_register(&at_direction);
    at_register(&at_building_action);

    callbacks.cast_spell = lua_callspell;
    callbacks.use_item = use_item_lua;
    callbacks.produce_resource = produce_resource_lua;
    callbacks.limit_resource = limit_resource_lua;

    register_function((pf_generic)lua_changeresource, "lua_changeresource");
    register_item_give(lua_giveitem, "lua_giveitem");
}
