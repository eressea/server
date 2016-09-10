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

#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/resolve.h>

#include <kernel/config.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/spell.h>
#include <kernel/race.h>
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

static int limit_resource(const region * r, const resource_type * rtype)
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
produce_resource(region * r, const resource_type * rtype, int norders)
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
static int lua_callspell(castorder * co)
{
    lua_State *L = (lua_State *)global.vm_state;
    const char *fname = co->sp->sname;
    unit *caster = co_get_caster(co);
    region * r = co_get_region(co);
    int result = -1;
    const char *hashpos = strchr(fname, '#');
    char fbuf[64];

    if (hashpos != NULL) {
        ptrdiff_t len = hashpos - fname;
        assert(len < (ptrdiff_t) sizeof(fbuf));
        strncpy(fbuf, fname, len);
        fbuf[len] = '\0';
        fname = fbuf;
    }

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

/** callback to initialize a familiar from lua. */
static int lua_initfamiliar(unit * u)
{
    lua_State *L = (lua_State *)global.vm_state;
    char fname[64];
    int result = -1;

    strlcpy(fname, "initfamiliar_", sizeof(fname));
    strlcat(fname, u_race(u)->_name, sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, u, TOLUA_CAST "unit");

        if (lua_pcall(L, 1, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("familiar(%s) calling '%s': %s.\n", unitname(u), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_warning("familiar(%s) calling '%s': not a function.\n", unitname(u), fname);
        lua_pop(L, 1);
    }

    create_mage(u, M_GRAY);

    strlcpy(fname, u_race(u)->_name, sizeof(fname));
    strlcat(fname, "_familiar", sizeof(fname));
    equip_unit(u, get_equipment(fname));
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

static int lua_getresource(unit * u, const struct resource_type *rtype)
{
    lua_State *L = (lua_State *)global.vm_state;
    int result = -1;
    char fname[64];

    strlcpy(fname, rtype->_name, sizeof(fname));
    strlcat(fname, "_getresource", sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, u, TOLUA_CAST "unit");

        if (lua_pcall(L, 1, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("get(%s) calling '%s': %s.\n", unitname(u), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("get(%s) calling '%s': not a function.\n", unitname(u), fname);
        lua_pop(L, 1);
    }

    return result;
}

static bool lua_canuse_item(const unit * u, const struct item_type *itype)
{
    bool result = true;
    lua_State *L = (lua_State *)global.vm_state;
    const char *fname = "item_canuse";

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");
        tolua_pushstring(L, itype->rtype->_name);

        if (lua_pcall(L, 2, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("use(%s) calling '%s': %s.\n", unitname(u), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = lua_toboolean(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("use(%s) calling '%s': not a function.\n", unitname(u), fname);
        lua_pop(L, 1);
    }
    return result;
}

static int
lua_wage(const region * r, const faction * f, const race * rc, int in_turn)
{
    lua_State *L = (lua_State *)global.vm_state;
    const char *fname = "wage";
    int result = -1;

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)r, TOLUA_CAST "region");
        tolua_pushusertype(L, (void *)f, TOLUA_CAST "faction");
        tolua_pushstring(L, rc ? rc->_name : 0);
        lua_pushinteger(L, in_turn);

        if (lua_pcall(L, 3, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("wage(%s) calling '%s': %s.\n", regionname(r, NULL), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("wage(%s) calling '%s': not a function.\n", regionname(r, NULL), fname);
        lua_pop(L, 1);
    }

    return result;
}

static void lua_agebuilding(building * b)
{
    lua_State *L = (lua_State *)global.vm_state;
    char fname[64];

    strlcpy(fname, "age_", sizeof(fname));
    strlcat(fname, b->type->_name, sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)b, TOLUA_CAST "building");

        if (lua_pcall(L, 1, 0, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("agebuilding(%s) calling '%s': %s.\n", buildingname(b), fname, error);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("agebuilding(%s) calling '%s': not a function.\n", buildingname(b), fname);
        lua_pop(L, 1);
    }
}

static double lua_building_taxes(building * b, int level)
{
    lua_State *L = (lua_State *)global.vm_state;
    const char *fname = "building_taxes";
    double result = 0.0F;

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)b, TOLUA_CAST "building");
        lua_pushinteger(L, level);

        if (lua_pcall(L, 2, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("building_taxes(%s) calling '%s': %s.\n", buildingname(b), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (double)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("building_taxes(%s) calling '%s': not a function.\n", buildingname(b), fname);
        lua_pop(L, 1);
    }
    return result;
}

static int lua_maintenance(const unit * u)
{
    lua_State *L = (lua_State *)global.vm_state;
    const char *fname = "maintenance";
    int result = -1;

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");

        if (lua_pcall(L, 1, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("maintenance(%s) calling '%s': %s.\n", unitname(u), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("maintenance(%s) calling '%s': not a function.\n", unitname(u), fname);
        lua_pop(L, 1);
    }

    return result;
}

static int lua_equipmentcallback(const struct equipment *eq, unit * u)
{
    lua_State *L = (lua_State *)global.vm_state;
    char fname[64];
    int result = -1;

    strlcpy(fname, "equip_", sizeof(fname));
    strlcat(fname, eq->name, sizeof(fname));

    lua_getglobal(L, fname);
    if (lua_isfunction(L, -1)) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");

        if (lua_pcall(L, 1, 1, 0) != 0) {
            const char *error = lua_tostring(L, -1);
            log_error("equip(%s) calling '%s': %s.\n", unitname(u), fname, error);
            lua_pop(L, 1);
        }
        else {
            result = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else {
        log_error("equip(%s) calling '%s': not a function.\n", unitname(u), fname);
        lua_pop(L, 1);
    }
    return result;
}

/** callback for an item-use function written in lua. */
int
lua_useitem(struct unit *u, const struct item_type *itype, int amount,
struct order *ord)
{
    lua_State *L = (lua_State *)global.vm_state;
    int result = 0;
    char fname[64];

    strlcpy(fname, "use_", sizeof(fname));
    strlcat(fname, itype->rtype->_name, sizeof(fname));

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
    }
    else {
        log_error("use(%s) calling '%s': not a function.\n", unitname(u), fname);
        lua_pop(L, 1);
    }

    return result;
}

int tolua_toid(lua_State * L, int idx, int def)
{
    int no = 0;
    int type = lua_type(L, idx);
    if (type == LUA_TNUMBER) {
        no = (int)tolua_tonumber(L, idx, def);
    }
    else {
        const char *str = tolua_tostring(L, idx, NULL);
        no = str ? atoi36(str) : def;
    }
    return no;
}

void register_tolua_helpers(void)
{
    at_register(&at_direction);
    at_register(&at_building_action);

    register_function((pf_generic)lua_building_taxes,
        TOLUA_CAST "lua_building_taxes");
    register_function((pf_generic)lua_agebuilding,
        TOLUA_CAST "lua_agebuilding");
    register_function((pf_generic)lua_callspell, TOLUA_CAST "lua_castspell");
    register_function((pf_generic)lua_initfamiliar,
        TOLUA_CAST "lua_initfamiliar");
    register_item_use(&lua_useitem, TOLUA_CAST "lua_useitem");
    register_function((pf_generic)lua_getresource,
        TOLUA_CAST "lua_getresource");
    register_function((pf_generic)lua_canuse_item,
        TOLUA_CAST "lua_canuse_item");
    register_function((pf_generic)lua_changeresource,
        TOLUA_CAST "lua_changeresource");
    register_function((pf_generic)lua_equipmentcallback,
        TOLUA_CAST "lua_equip");

    register_function((pf_generic)lua_wage, TOLUA_CAST "lua_wage");
    register_function((pf_generic)lua_maintenance,
        TOLUA_CAST "lua_maintenance");

    register_function((pf_generic)produce_resource,
        TOLUA_CAST "lua_produceresource");
    register_function((pf_generic)limit_resource,
        TOLUA_CAST "lua_limitresource");
    register_item_give(lua_giveitem, TOLUA_CAST "lua_giveitem");
}
