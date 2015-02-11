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
#include "bind_region.h"
#include "bind_unit.h"
#include "bind_ship.h"
#include "bind_dict.h"
#include "bind_building.h"

#include "chaos.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/item.h>
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/plane.h>
#include <kernel/terrain.h>
#include <modules/autoseed.h>
#include <attributes/key.h>
#include <attributes/racename.h>

#include <util/attrib.h>
#include <util/base36.h>
#include <critbit.h>
#include <util/language.h>
#include <util/log.h>

#include <tolua.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

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
    region *self = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushnumber(L, (lua_Number)self->uid);
    return 1;
}

static int tolua_region_get_blocked(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    lua_pushboolean(L, (self->flags&RF_BLOCKED) != 0);
    return 1;
}

static int tolua_region_set_blocked(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    bool flag = !!tolua_toboolean(L, 2, 1);
    if (flag) self->flags |= BLD_WORKING;
    else self->flags &= ~BLD_WORKING;
    return 0;
}

static int tolua_region_get_x(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushnumber(L, (lua_Number)self->x);
    return 1;
}

static int tolua_region_get_y(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushnumber(L, (lua_Number)self->y);
    return 1;
}

static int tolua_region_get_plane(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, rplane(r), TOLUA_CAST "plane");
    return 1;
}

static int tolua_region_get_terrain(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, self->terrain->_name);
    return 1;
}

static int tolua_region_set_terrain(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    const char *tname = tolua_tostring(L, 2, 0);
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
    region *self = (region *)tolua_tousertype(L, 1, 0);
    attrib *a = a_find(self->attribs, &at_racename);
    if (a) {
        tolua_pushstring(L, get_racename(a));
        return 1;
    }
    return 0;
}

static int tolua_region_set_owner(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    struct faction *f = (struct faction *)tolua_tousertype(L, 2, 0);
    if (r) {
        region_set_owner(r, f, turn);
    }
    return 0;
}

static int tolua_region_get_owner(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    if (r) {
        struct faction *f = region_get_owner(r);
        tolua_pushusertype(L, f, TOLUA_CAST "faction");
        return 1;
    }
    return 0;
}

static int tolua_region_set_terrainname(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
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
    region *self = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, region_getinfo(self));
    return 1;
}

static int tolua_region_set_info(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    region_setinfo(self, tolua_tostring(L, 2, 0));
    return 0;
}

static int tolua_region_get_name(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, region_getname(self));
    return 1;
}

static int tolua_region_set_name(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    region_setname(self, tolua_tostring(L, 2, 0));
    return 0;
}

static int tolua_region_get_morale(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushnumber(L, region_get_morale(r));
    return 1;
}

static int tolua_region_set_morale(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    region_set_morale(r, (int)tolua_tonumber(L, 2, 0), turn);
    return 0;
}

static int tolua_region_get_adj(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
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
    region *r = (region *)tolua_tousertype(L, 1, 0);
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
    region *r = (region *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
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
    region *r = (region *)tolua_tousertype(L, 1, 0);
    if (r->land) {
        const char *name = tolua_tostring(L, 2, 0);
        const item_type *itype = it_find(name);
        if (itype && (itype->flags & ITF_HERB)) {
            r->land->herbtype = itype;
        }
    }
    return 0;
}

static int tolua_region_get_herb(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    if (r->land && r->land->herbtype) {
        const char *name = r->land->herbtype->rtype->_name;
        tolua_pushstring(L, name);
        return 1;
    }
    return 0;
}

static int tolua_region_get_next(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    direction_t dir = (direction_t)tolua_tonumber(L, 2, 0);

    if (dir >= 0 && dir < MAXDIRECTIONS) {
        tolua_pushusertype(L, (void *)r_connect(self, dir), TOLUA_CAST "region");
        return 1;
    }
    return 0;
}

static int tolua_region_get_flag(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    int bit = (int)tolua_tonumber(L, 2, 0);

    lua_pushboolean(L, (self->flags & (1 << bit)));
    return 1;
}

static int tolua_region_set_flag(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
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
    region *r = (region *)tolua_tousertype(L, 1, 0);
    const char *type = tolua_tostring(L, 2, 0);
    const resource_type *rtype = rt_find(type);
    if (rtype != NULL) {
        const rawmaterial *rm;
        for (rm = r->resources; rm; rm = rm->next) {
            if (rm->type->rtype == rtype) {
                tolua_pushnumber(L, (lua_Number)rm->level);
                return 1;
            }
        }
    }
    return 0;
}

#define LUA_ASSERT(c, s) if (!(c)) { log_error("%s(%d): %s\n", __FILE__, __LINE__, (s)); return 0; }

static critbit_tree * special_resources(void)
{
    static critbit_tree cb = CRITBIT_TREE();
    if (!cb.root) {
        const char * special[] = { "seed", "sapling", "tree", "grave", "chaos", 0 };
        char buffer[32];
        int i;
        for (i = 0; special[i]; ++i) {
            size_t len = strlen(special[i]);
            len = cb_new_kv(special[i], len, &i, sizeof(int), buffer);
            cb_insert(&cb, buffer, len);
        }
    }
    return &cb;
}

static int tolua_region_get_resource(lua_State * L)
{
    region *r;
    const char *type;
    const resource_type *rtype;
    int result = 0;
    const void * matches;
    critbit_tree * cb = special_resources();

    r = (region *)tolua_tousertype(L, 1, 0);
    LUA_ASSERT(r != NULL, "invalid parameter");
    type = tolua_tostring(L, 2, 0);
    LUA_ASSERT(type != NULL, "invalid parameter");

    if (cb_find_prefix(cb, type, strlen(type) + 1, &matches, 1, 0)) {
        cb_get_kv(matches, &result, sizeof(result));
        switch (result) {
        case 0:
        case 1:
        case 2:
            result = rtrees(r, result);
            break;
        case 3:
            result = deathcount(r);
            break;
        case 4:
            result = get_chaoscount(r);
            break;
        }
    }
    else {
        rtype = rt_find(type);
        if (rtype) {
            result = region_getresource(r, rtype);
        }
        else {
            result = -1;
        }
    }

    tolua_pushnumber(L, (lua_Number)result);
    return 1;
}

static int tolua_region_set_resource(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    const char *type = tolua_tostring(L, 2, 0);
    int result, value = (int)tolua_tonumber(L, 3, 0);
    critbit_tree * cb = special_resources();
    const void * matches;

    if (cb_find_prefix(cb, type, strlen(type) + 1, &matches, 1, 0)) {
        cb_get_kv(matches, &result, sizeof(result));
        switch (result) {
        case 0:
        case 1:
        case 2:
            rsettrees(r, result, value);
            break;
        case 3:
            deathcounts(r, value - deathcount(r));
            break;
        case 4:
            add_chaoscount(r, value - get_chaoscount(r));
            break;
        }
    }
    else {
        const resource_type *rtype = rt_find(type);
        if (rtype != NULL) {
            region_setresource(r, rtype, value);
        }
    }
    return 0;
}

static int tolua_region_get_objects(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, (void *)&self->attribs, USERTYPE_DICT);
    return 1;
}

static int tolua_region_destroy(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    remove_region(&regions, self);
    return 0;
}

static int tolua_region_create(lua_State * L)
{
    int x = (int)tolua_tonumber(L, 1, 0);
    int y = (int)tolua_tonumber(L, 2, 0);
    const char *tname = tolua_tostring(L, 3, 0);
    if (tname) {
        plane *pl = findplane(x, y);
        const terrain_type *terrain = get_terrain(tname);
        region *r, *result;
        if (!terrain) {
            return 0;
        }

        assert(!pnormalize(&x, &y, pl));
        r = result = findregion(x, y);

        if (terrain == NULL && r != NULL && r->units != NULL) {
            /* TODO: error message */
            result = NULL;
        }
        else if (r == NULL) {
            result = new_region(x, y, pl, 0);
        }
        if (result) {
            terraform_region(result, terrain);
        }
        if (result->land) {
            fix_demand(result);
        }

        tolua_pushusertype(L, result, TOLUA_CAST "region");
        return 1;
    }
    return 0;
}

static int tolua_region_get_units(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    unit **unit_ptr = (unit **)lua_newuserdata(L, sizeof(unit *));

    luaL_getmetatable(L, "unit");
    lua_setmetatable(L, -2);

    *unit_ptr = self->units;

    lua_pushcclosure(L, tolua_unitlist_next, 1);
    return 1;
}

static int tolua_region_get_buildings(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
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
    region *self = (region *)tolua_tousertype(L, 1, 0);
    ship **ship_ptr = (ship **)lua_newuserdata(L, sizeof(ship *));

    luaL_getmetatable(L, "ship");
    lua_setmetatable(L, -2);

    *ship_ptr = self->ships;

    lua_pushcclosure(L, tolua_shiplist_next, 1);
    return 1;
}

static int tolua_region_get_age(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);

    if (self) {
        lua_pushnumber(L, self->age);
        return 1;
    }
    return 0;
}

static int tolua_region_getkey(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);

    int flag = atoi36(name);
    attrib *a = find_key(self->attribs, flag);
    lua_pushboolean(L, a != NULL);

    return 1;
}

static int tolua_region_setkey(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
    int value = tolua_toboolean(L, 3, 0);

    int flag = atoi36(name);
    attrib *a = find_key(self->attribs, flag);
    if (a == NULL && value) {
        add_key(&self->attribs, flag);
    }
    else if (a != NULL && !value) {
        a_remove(&self->attribs, a);
    }
    return 0;
}

static int tolua_region_tostring(lua_State * L)
{
    region *self = (region *)tolua_tousertype(L, 1, 0);
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

static int tolua_plane_create(lua_State * L)
{
    int id = (int)tolua_tonumber(L, 1, 0);
    int x = (int)tolua_tonumber(L, 2, 0);
    int y = (int)tolua_tonumber(L, 3, 0);
    int width = (int)tolua_tonumber(L, 4, 0);
    int height = (int)tolua_tonumber(L, 5, 0);
    const char *name = tolua_tostring(L, 6, 0);
    plane *pl;

    pl = create_new_plane(id, name, x, x + width - 1, y, y + height - 1, 0);

    tolua_pushusertype(L, pl, TOLUA_CAST "plane");
    return 1;
}

static int tolua_plane_get_name(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, self->name);
    return 1;
}

static int tolua_plane_set_name(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, 0);
    const char *str = tolua_tostring(L, 2, 0);
    free(self->name);
    if (str)
        self->name = _strdup(str);
    else
        self->name = 0;
    return 0;
}

static int tolua_plane_get_id(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, 0);
    tolua_pushnumber(L, (lua_Number)self->id);
    return 1;
}

static int tolua_plane_normalize(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, 0);
    int x = (int)tolua_tonumber(L, 2, 0);
    int y = (int)tolua_tonumber(L, 3, 0);
    pnormalize(&x, &y, self);
    tolua_pushnumber(L, (lua_Number)x);
    tolua_pushnumber(L, (lua_Number)y);
    return 2;
}

static int tolua_plane_tostring(lua_State * L)
{
    plane *self = (plane *)tolua_tousertype(L, 1, 0);
    lua_pushstring(L, self->name);
    return 1;
}

static int tolua_plane_get_size(lua_State * L)
{
    plane *pl = (plane *)tolua_tousertype(L, 1, 0);
    lua_pushnumber(L, plane_width(pl));
    lua_pushnumber(L, plane_height(pl));
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
    lua_pushnumber(L, result);
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
            tolua_variable(L, TOLUA_CAST "info", tolua_region_get_info,
                tolua_region_set_info);
            tolua_variable(L, TOLUA_CAST "units", tolua_region_get_units, NULL);
            tolua_variable(L, TOLUA_CAST "ships", tolua_region_get_ships, NULL);
            tolua_variable(L, TOLUA_CAST "age", tolua_region_get_age, NULL);
            tolua_variable(L, TOLUA_CAST "buildings", tolua_region_get_buildings,
                NULL);
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

            tolua_function(L, TOLUA_CAST "get_key", tolua_region_getkey);
            tolua_function(L, TOLUA_CAST "set_key", tolua_region_setkey);

            tolua_variable(L, TOLUA_CAST "objects", tolua_region_get_objects, 0);
        }
        tolua_endmodule(L);

        tolua_cclass(L, TOLUA_CAST "plane", TOLUA_CAST "plane", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "plane");
        {
            tolua_function(L, TOLUA_CAST "create", tolua_plane_create);
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
