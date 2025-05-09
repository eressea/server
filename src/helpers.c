#include "attributes/dict.h"

#include "helpers.h"
#include "vortex.h"
#include "magic.h"

#include <util/functions.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/parser.h>
#include <util/variant.h>

#include <kernel/attrib.h>
#include <kernel/config.h>
#include <kernel/callbacks.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

#include <storage.h>

#include <stb_ds.h>
#include <tolua.h>
#include <lua.h>

#include <stdbool.h>
#include <stdio.h>

struct order;

static int limit_resource_lua(const region * r, const resource_type * rtype)
{
    char fname[64];
    int result = -1, len;
    lua_State *L = (lua_State *)global.vm_state;

    len = snprintf(fname, sizeof(fname), "%s_limit", rtype->_name);
    if (len > 0 && (size_t)len < sizeof(fname)) {
        lua_getglobal(L, fname);
        if (lua_isfunction(L, -1)) {
            tolua_pushusertype(L, (void *)r, "region");

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
    }
    return result;
}

static void
produce_resource_lua(region * r, const resource_type * rtype, int norders)
{
    lua_State *L = (lua_State *)global.vm_state;
    char fname[64];
    int len;

    len = snprintf(fname, sizeof(fname), "%s_produce", rtype->_name);
    if (len > 0 && (size_t)len < sizeof(fname)) {
        lua_getglobal(L, fname);
        if (lua_isfunction(L, -1)) {
            tolua_pushusertype(L, (void *)r, "region");
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
}

static void push_param(lua_State * L, char c, spellparameter* param)
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
        tolua_pushstring(L, param->data.xs);
    else {
        log_error("unsupported syntax %c.\n", c);
        lua_pushnil(L);
    }
}

/** callback to use lua functions isntead of equipment */
static bool lua_equipunit(unit *u, const char *eqname, int mask) {
    lua_State *L = (lua_State *)global.vm_state;
    bool result = false;
    static bool disabled = false;

    if (disabled) {
        return false;
    }
    lua_getglobal(L, "equip_unit");
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, u, "unit");
        lua_pushstring(L, eqname);
        lua_pushinteger(L, mask);
        if (lua_pcall(L, 3, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("equip(%s) with '%s/%d': %s.\n", unitname(u), eqname, mask, error);
            lua_pop(L, 1);
        }
        else {
            result = lua_toboolean(L, -1) != 0;
            lua_pop(L, 1);
        }
    }
    else {
        disabled = true;
    }
    return result;
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
        tolua_pushusertype(L, r, "region");
        tolua_pushusertype(L, caster, "unit");
        lua_pushinteger(L, co->level);
        lua_pushnumber(L, co->force);
        if (co->sp->parameter && co->a_params) {
            const char *synp = co->sp->parameter;
            size_t i = 0, len = arrlen(co->a_params);
            ++nparam;
            lua_newtable(L);
            while (*synp && i < len) {
                spellparameter *param = co->a_params + i;
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
    int len, result = -1;
    char fname[64];

    len = snprintf(fname, sizeof(fname), "%s_changeresource", rtype->_name);
    if (len > 0 && (size_t)len < sizeof(fname)) {
        lua_getglobal(L, fname);
        if (lua_isfunction(L, -1)) {
            tolua_pushusertype(L, u, "unit");
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
    }
    return result;
}

/** callback for an item-use function written in lua. */
static int
lua_use_item(unit *u, const item_type *itype, int amount, struct order *ord)
{
    int len;
    char fname[64];

    len = snprintf(fname, sizeof(fname), "use_%s", itype->rtype->_name);
    if (len > 0 && (size_t)len < sizeof(fname)) {
        lua_State *L = (lua_State *)global.vm_state;
        lua_getglobal(L, fname);
        if (lua_isfunction(L, -1)) {
            tolua_pushusertype(L, (void *)u, "unit");
            lua_pushinteger(L, amount);
            lua_pushstring(L, getstrtoken());
            tolua_pushusertype(L, (void *)ord, "order");
            if (lua_pcall(L, 4, 1, 0) != 0) {
                const char *error = lua_tostring(L, -1);
                log_error("use(%s) calling '%s': %s.\n", unitname(u), fname, error);
            }
            else {
                int result = (int)lua_tonumber(L, -1);
                lua_pop(L, 1);
                return result;
            }
        }
        else {
            log_error("use_item(%s) calling '%s': not a function.\n", unitname(u), fname);
        }
        lua_pop(L, 1);
    }
    return EUNUSABLE;
}

/* compat code for old data files */
static int caldera_read(trigger *t, gamedata *data)
{
    UNUSED_ARG(t);
    READ_INT(data->store, NULL);
    return AT_READ_FAIL;
}

struct trigger_type tt_caldera = {
    "caldera",
    NULL, NULL, NULL, NULL,
    caldera_read
};


static int building_action_read(variant *var, void *owner, gamedata *data)
{
    struct storage *store = data->store;

    UNUSED_ARG(owner);
    UNUSED_ARG(var);

    if (data->version < ATTRIBOWNER_VERSION) {
        READ_INT(data->store, NULL);
    }
    READ_TOK(store, NULL, 0);
    READ_TOK(store, NULL, 0);
    return AT_READ_DEPR;
}

void register_tolua_helpers(void)
{
    tt_register(&tt_caldera);
    at_register(&at_direction);
    at_register(&at_dict); /* deprecated, but has an upgrade path */
    at_deprecate("lcbuilding", building_action_read);

    callbacks.equip_unit = lua_equipunit;
    callbacks.cast_spell = lua_callspell;
    callbacks.use_item = lua_use_item;
    callbacks.produce_resource = produce_resource_lua;
    callbacks.limit_resource = limit_resource_lua;

    register_function((pf_generic)lua_changeresource, "lua_changeresource");
}
