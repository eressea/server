#ifdef _MSC_VER
#include <platform.h>
#endif

#include "bind_region.h"
#include "bind_unit.h"
#include "bind_ship.h"
#include "bind_building.h"

#include "teleport.h"
#include "direction.h"

#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/curse.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/unit.h>
#include <kernel/item.h>
#include <kernel/ship.h>
#include <kernel/plane.h>
#include <kernel/terrain.h>
#include <kernel/messages.h>

#include <kernel/attrib.h>
#include <util/base36.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/strings.h>

#include <attributes/key.h>
#include <attributes/racename.h>

#include <lua.h>
#include <lauxlib.h>
#include <tolua.h>

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

static int tolua_region_count_msg_type(lua_State *L) {
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    const char *str = tolua_tostring(L, 2, NULL);
    int n = 0;
    if (self->msgs) {
        mlist * ml = self->msgs->begin;
        while (ml) {
            if (strcmp(str, ml->msg->type->name) == 0) {
                ++n;
            }
            ml = ml->next;
        }
    }
    lua_pushinteger(L, n);
    return 1;
}

int tolua_regionlist_next(lua_State * L)
{
    region **region_ptr = (region **)lua_touserdata(L, lua_upvalueindex(1));
    region *r = *region_ptr;
    if (r != NULL) {
        tolua_pushusertype(L, (void *)r, TOLUA_CAST "region");
        *region_ptr = r->next;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

static int tolua_region_get_id(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->uid);
    return 1;
}

static int tolua_region_get_blocked(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    lua_pushboolean(L, (self->flags&RF_BLOCKED) != 0);
    return 1;
}

static int tolua_region_set_blocked(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    bool flag = !!tolua_toboolean(L, 2, 1);
    if (flag) self->flags |= RF_BLOCKED;
    else self->flags &= ~RF_BLOCKED;
    return 0;
}

static int tolua_region_get_x(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->x);
    return 1;
}

static int tolua_region_get_y(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->y);
    return 1;
}

static int tolua_region_get_plane(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    tolua_pushusertype(L, rplane(r), TOLUA_CAST "plane");
    return 1;
}

static int tolua_region_get_terrain(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, self->terrain->_name);
    return 1;
}

static int tolua_region_set_terrain(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    const char *tname = tolua_tostring(L, 2, NULL);
    if (tname) {
        const terrain_type *terrain = get_terrain(tname);
        if (terrain) {
            terraform_region(r, terrain);
        }
    }
    return 0;
}

static int tolua_region_get_terrainname(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    attrib *a = a_find(self->attribs, &at_racename);
    if (a) {
        tolua_pushstring(L, get_racename(a));
        return 1;
    }
    return 0;
}

static int tolua_region_set_owner(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    struct faction *f = (struct faction *)tolua_tousertype(L, 2, NULL);
    if (r) {
        region_set_owner(r, f, turn);
    }
    return 0;
}

static int tolua_region_get_owner(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    if (r) {
        struct faction *f = region_get_owner(r);
        tolua_pushusertype(L, f, TOLUA_CAST "faction");
        return 1;
    }
    return 0;
}

static int tolua_region_set_terrainname(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    if (name == NULL) {
        a_removeall(&self->attribs, &at_racename);
    }
    else {
        set_racename(&self->attribs, name);
    }
    return 0;
}

static int tolua_region_get_info(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, region_getinfo(self));
    return 1;
}

static int tolua_region_set_info(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    region_setinfo(self, tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_region_get_name(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, region_getname(self));
    return 1;
}

static int tolua_region_set_name(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    region_setname(self, tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_region_get_morale(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, region_get_morale(r));
    return 1;
}

static int tolua_region_set_morale(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    region_set_morale(r, (int)tolua_tonumber(L, 2, 0), turn);
    return 0;
}

/* region mourning this turn */
static int tolua_region_get_is_mourning(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    lua_pushboolean(L, is_mourning(r, turn+1));
    return 1;
}

static int tolua_region_get_adj(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    region *rn[MAXDIRECTIONS];
    int d, idx;
    get_neighbours(r, rn);

    lua_createtable(L, MAXDIRECTIONS, 0);
    for (d = 0, idx = 0; d != MAXDIRECTIONS; ++d) {
        if (rn[d]) {
            tolua_pushusertype(L, rn[d], TOLUA_CAST "region");
            lua_rawseti(L, -2, ++idx);
        }
    }
    return 1;
}

static int tolua_region_get_luxury(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    if (r->land) {
        const item_type *lux = r_luxury(r);
        if (lux) {
            const char *name = lux->rtype->_name;
            tolua_pushstring(L, name);
            return 1;
        }
    }
    return 0;
}

static int tolua_region_set_luxury(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    if (r->land && name) {
        const item_type *lux = r_luxury(r);
        const item_type *itype = it_find(name);
        if (lux && itype && lux != itype) {
            r_setdemand(r, lux->rtype->ltype, 1);
            r_setdemand(r, itype->rtype->ltype, 0);
        }
    }
    return 0;
}

static int tolua_region_set_herb(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    if (r->land) {
        const char *name = tolua_tostring(L, 2, NULL);
        const item_type *itype = it_find(name);
        if (itype && (itype->flags & ITF_HERB)) {
            r->land->herbtype = itype;
        }
    }
    return 0;
}

static int tolua_region_get_herb(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    if (r->land && r->land->herbtype) {
        const char *name = r->land->herbtype->rtype->_name;
        tolua_pushstring(L, name);
        return 1;
    }
    return 0;
}

static int tolua_region_get_next(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    direction_t dir = (direction_t)tolua_tonumber(L, 2, 0);

    if (dir >= 0 && dir < MAXDIRECTIONS) {
        tolua_pushusertype(L, (void *)r_connect(self, dir), TOLUA_CAST "region");
        return 1;
    }
    return 0;
}

static int tolua_region_get_flag(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    int bit = (int)tolua_tonumber(L, 2, 0);

    lua_pushboolean(L, (self->flags & (1 << bit)));
    return 1;
}

static int tolua_region_set_flag(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    int bit = (int)tolua_tonumber(L, 2, 0);
    int set = tolua_toboolean(L, 3, 1);

    if (set)
        self->flags |= (1 << bit);
    else
        self->flags &= ~(1 << bit);
    return 0;
}

static int tolua_region_get_resourcelevel(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    const char *type = tolua_tostring(L, 2, NULL);
    const resource_type *rtype = rt_find(type);
    if (rtype != NULL) {
        const rawmaterial *rm;
        for (rm = r->resources; rm; rm = rm->next) {
            if (rm->rtype == rtype) {
                lua_pushinteger(L, rm->level);
                return 1;
            }
        }
    }
    return 0;
}

#define LUA_ASSERT(c, s) if (!(c)) { log_error("%s(%d): %s\n", __FILE__, __LINE__, (s)); return 0; }

static int special_resource(const char *type) {
    const char * special[] = { "seed", "sapling", "tree", "grave", NULL };
    int i;

    for (i = 0; special[i]; ++i) {
        if (strcmp(type, special[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int tolua_region_get_resource(lua_State * L)
{
    region *r;
    const char *type;
    const resource_type *rtype;
    int result;

    r = (region *)tolua_tousertype(L, 1, NULL);
    LUA_ASSERT(r != NULL, "invalid parameter");
    type = tolua_tostring(L, 2, NULL);
    LUA_ASSERT(type != NULL, "invalid parameter");
    
    result = special_resource(type);
    switch (result) {
    case 0:
    case 1:
    case 2:
        result = rtrees(r, result);
        break;
    case 3:
        result = deathcount(r);
        break;
    default:
        rtype = rt_find(type);
        if (rtype) {
            result = region_getresource(r, rtype);
        }
        else {
            result = -1;
        }
    }

    lua_pushinteger(L, result);
    return 1;
}

static int tolua_region_set_resource(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    const char *type = tolua_tostring(L, 2, NULL);
    int result, value = (int)tolua_tonumber(L, 3, 0);
    const resource_type *rtype;

    result = special_resource(type);
    switch (result) {
    case 0:
    case 1:
    case 2:
        rsettrees(r, result, value);
        break;
    case 3:
        deathcounts(r, value - deathcount(r));
        break;
    default:
        rtype = rt_find(type);
        if (rtype != NULL) {
            region_setresource(r, rtype, value);
        }
    }
    return 0;
}

static int tolua_region_destroy(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    remove_region(&regions, self);
    return 0;
}

static int tolua_region_create(lua_State * L)
{
    int x = (int)tolua_tonumber(L, 1, 0);
    int y = (int)tolua_tonumber(L, 2, 0);
    const char *tname = tolua_tostring(L, 3, NULL);
    if (tname) {
        plane *pl = findplane(x, y);
        const terrain_type *terrain = get_terrain(tname);
        region *r, *result;
        if (!terrain) {
            log_error("lua: region.create with invalid terrain %s", tname);
            return 0;
        }

        pnormalize(&x, &y, pl);
        r = result = findregion(x, y);

        if (r != NULL && r->units != NULL) {
            /* TODO: error message */
            result = NULL;
        }
        else if (r == NULL) {
            result = new_region(x, y, pl, 0);
        }
        if (result) {
            terraform_region(result, terrain);
        }

        tolua_pushusertype(L, result, TOLUA_CAST "region");
        return 1;
    }
    log_error("lua: region.create with invalid terrain %s", tname);
    return 0;
}

static int tolua_region_get_units(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    unit **unit_ptr = (unit **)lua_newuserdata(L, sizeof(unit *));

    luaL_getmetatable(L, "unit");
    lua_setmetatable(L, -2);

    *unit_ptr = self->units;

    lua_pushcclosure(L, tolua_unitlist_next, 1);
    return 1;
}

static int tolua_region_get_buildings(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    building **building_ptr =
        (building **)lua_newuserdata(L, sizeof(building *));

    luaL_getmetatable(L, "building");
    lua_setmetatable(L, -2);

    *building_ptr = self->buildings;

    lua_pushcclosure(L, tolua_buildinglist_next, 1);
    return 1;
}

static int tolua_region_get_ships(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    ship **ship_ptr = (ship **)lua_newuserdata(L, sizeof(ship *));

    luaL_getmetatable(L, "ship");
    lua_setmetatable(L, -2);

    *ship_ptr = self->ships;

    lua_pushcclosure(L, tolua_shiplist_next, 1);
    return 1;
}

static int tolua_region_get_age(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);

    if (self) {
        lua_pushinteger(L, self->age);
        return 1;
    }
    return 0;
}

static int tolua_region_get_peasants(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);

    if (self) {
        lua_pushinteger(L, self->land ? self->land->peasants : 0);
        return 1;
    }
    return 0;
}

static int tolua_region_set_peasants(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);

    if (self && self->land) {
        self->land->peasants = lua_tointeger(L, 2);
    }
    return 0;
}

static int tolua_region_getkey(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    int flag = atoi36(name);
    int value = key_get(self->attribs, flag);
    if (value != 0) {
        lua_pushinteger(L, value);
        return 1;
    }
    return 0;
}

static int tolua_region_setkey(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    int value = (int)tolua_tonumber(L, 3, 1);
    int flag = atoi36(name);

    if (value) {
        key_set(&self->attribs, flag, value);
    }
    else {
        key_unset(&self->attribs, flag);
    }
    return 0;
}

static int tolua_region_getastral(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, NULL);
    region *rt = r_standard_to_astral(r);

    if (!rt) {
        const char *tname = tolua_tostring(L, 2, NULL);
        plane *pl = get_astralplane();
        rt = new_region(real2tp(r->x), real2tp(r->y), pl, 0);
        if (tname) {
            const terrain_type *terrain = get_terrain(tname);
            terraform_region(rt, terrain);
        }
    }
    tolua_pushusertype(L, rt, TOLUA_CAST "region");
    return 1;
}

static int tolua_region_tostring(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, regionname(self, NULL));
    return 1;
}

static int tolua_plane_get(lua_State * L)
{
    int id = (int)tolua_tonumber(L, 1, 0);
    plane *pl = getplanebyid(id);

    tolua_pushusertype(L, pl, TOLUA_CAST "plane");
    return 1;
}

static int tolua_plane_erase(lua_State *L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, NULL);
    remove_plane(self);
    return 0;
}

static int tolua_plane_create(lua_State * L)
{
    int id = (int)tolua_tonumber(L, 1, 0);
    int x = (int)tolua_tonumber(L, 2, 0);
    int y = (int)tolua_tonumber(L, 3, 0);
    int width = (int)tolua_tonumber(L, 4, 0);
    int height = (int)tolua_tonumber(L, 5, 0);
    const char *name = tolua_tostring(L, 6, NULL);
    plane *pl;

    pl = create_new_plane(id, name, x, x + width - 1, y, y + height - 1, 0);

    tolua_pushusertype(L, pl, TOLUA_CAST "plane");
    return 1;
}

static int tolua_plane_get_name(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, self->name);
    return 1;
}

static int tolua_plane_set_name(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, NULL);
    const char *str = tolua_tostring(L, 2, NULL);
    free(self->name);
    if (str)
        self->name = str_strdup(str);
    else
        self->name = 0;
    return 0;
}

static int tolua_plane_get_id(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->id);
    return 1;
}

static int tolua_plane_normalize(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, NULL);
    int x = (int)tolua_tonumber(L, 2, 0);
    int y = (int)tolua_tonumber(L, 3, 0);
    pnormalize(&x, &y, self);
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static int tolua_plane_tostring(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, self->name);
    return 1;
}

static int tolua_plane_get_size(lua_State * L)
{
    plane *pl = (plane *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, plane_width(pl));
    lua_pushinteger(L, plane_height(pl));
    return 2;
}

static int tolua_distance(lua_State * L)
{
    int x1 = (int)tolua_tonumber(L, 1, 0);
    int y1 = (int)tolua_tonumber(L, 2, 0);
    int x2 = (int)tolua_tonumber(L, 3, 0);
    int y2 = (int)tolua_tonumber(L, 4, 0);
    plane *pl = (plane *)tolua_tousertype(L, 5, 0);
    int result;

    if (!pl)
        pl = get_homeplane();
    pnormalize(&x1, &y1, pl);
    pnormalize(&x2, &y2, pl);
    result = koor_distance(x1, y1, x2, y2);
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_region_get_curse(lua_State *L) {
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    if (self->attribs) {
        curse * c = get_curse(self->attribs, ct_find(name));
        if (c) {
            lua_pushnumber(L, curse_geteffect(c));
            return 1;
        }
    }
    return 0;
}

static int tolua_region_has_attrib(lua_State *L) {
    region *self = (region *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    attrib * a = a_find(self->attribs, at_find(name));
    lua_pushboolean(L, a != NULL);
    return 1;
}

void tolua_region_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "region");
    tolua_usertype(L, TOLUA_CAST "plane");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_function(L, TOLUA_CAST "distance", tolua_distance);

        tolua_cclass(L, TOLUA_CAST "region", TOLUA_CAST "region", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "region");
        {
            tolua_function(L, TOLUA_CAST "create", tolua_region_create);
            tolua_function(L, TOLUA_CAST "destroy", tolua_region_destroy);
            tolua_function(L, TOLUA_CAST "__tostring", tolua_region_tostring);

            tolua_function(L, TOLUA_CAST "count_msg_type", tolua_region_count_msg_type);

            tolua_function(L, TOLUA_CAST "get_curse", &tolua_region_get_curse);
            tolua_function(L, TOLUA_CAST "has_attrib", &tolua_region_has_attrib);
            /* flags */
            tolua_variable(L, TOLUA_CAST "blocked", tolua_region_get_blocked, tolua_region_set_blocked);

            tolua_variable(L, TOLUA_CAST "id", tolua_region_get_id, NULL);
            tolua_variable(L, TOLUA_CAST "x", tolua_region_get_x, NULL);
            tolua_variable(L, TOLUA_CAST "y", tolua_region_get_y, NULL);
            tolua_variable(L, TOLUA_CAST "plane", tolua_region_get_plane, NULL);
            tolua_variable(L, TOLUA_CAST "name", tolua_region_get_name,
                tolua_region_set_name);
            tolua_variable(L, TOLUA_CAST "morale", tolua_region_get_morale,
                tolua_region_set_morale);
            tolua_variable(L, TOLUA_CAST "is_mourning", tolua_region_get_is_mourning, NULL);
            tolua_variable(L, TOLUA_CAST "info", tolua_region_get_info,
                tolua_region_set_info);
            tolua_variable(L, TOLUA_CAST "units", tolua_region_get_units, NULL);
            tolua_variable(L, TOLUA_CAST "ships", tolua_region_get_ships, NULL);
            tolua_variable(L, TOLUA_CAST "age", tolua_region_get_age, NULL);
            tolua_variable(L, TOLUA_CAST "buildings", tolua_region_get_buildings,
                NULL);
            tolua_variable(L, TOLUA_CAST "peasants", tolua_region_get_peasants,
                tolua_region_set_peasants);
            tolua_variable(L, TOLUA_CAST "terrain", tolua_region_get_terrain,
                tolua_region_set_terrain);
            tolua_function(L, TOLUA_CAST "get_resourcelevel",
                tolua_region_get_resourcelevel);
            tolua_function(L, TOLUA_CAST "get_resource", tolua_region_get_resource);
            tolua_function(L, TOLUA_CAST "set_resource", tolua_region_set_resource);
            tolua_function(L, TOLUA_CAST "get_flag", tolua_region_get_flag);
            tolua_function(L, TOLUA_CAST "set_flag", tolua_region_set_flag);
            tolua_function(L, TOLUA_CAST "next", tolua_region_get_next);
            tolua_variable(L, TOLUA_CAST "adj", tolua_region_get_adj, NULL);

            tolua_variable(L, TOLUA_CAST "luxury", &tolua_region_get_luxury,
                &tolua_region_set_luxury);
            tolua_variable(L, TOLUA_CAST "herb", &tolua_region_get_herb,
                &tolua_region_set_herb);

            tolua_variable(L, TOLUA_CAST "terrain_name",
                &tolua_region_get_terrainname, &tolua_region_set_terrainname);
            tolua_variable(L, TOLUA_CAST "owner", &tolua_region_get_owner,
                &tolua_region_set_owner);

            tolua_function(L, TOLUA_CAST "get_astral", tolua_region_getastral);

            tolua_function(L, TOLUA_CAST "get_key", tolua_region_getkey);
            tolua_function(L, TOLUA_CAST "set_key", tolua_region_setkey);
        }
        tolua_endmodule(L);

        tolua_cclass(L, TOLUA_CAST "plane", TOLUA_CAST "plane", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "plane");
        {
            tolua_function(L, TOLUA_CAST "create", tolua_plane_create);
            tolua_function(L, TOLUA_CAST "erase", tolua_plane_erase);
            tolua_function(L, TOLUA_CAST "get", tolua_plane_get);
            tolua_function(L, TOLUA_CAST "__tostring", tolua_plane_tostring);

            tolua_function(L, TOLUA_CAST "size", tolua_plane_get_size);
            tolua_variable(L, TOLUA_CAST "id", tolua_plane_get_id, NULL);
            tolua_function(L, TOLUA_CAST "normalize", tolua_plane_normalize);
            tolua_variable(L, TOLUA_CAST "name", tolua_plane_get_name,
                tolua_plane_set_name);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
