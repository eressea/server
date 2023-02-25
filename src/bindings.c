#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "bindings.h"
#include "bind_tolua.h"

#include "console.h"
#include "gamedb.h"
#include "helpers.h"
#include "laws.h"
#include "magic.h"
#include "orderfile.h"
#include "reports.h"
#include "summary.h"
#include "teleport.h"

#include "kernel/attrib.h"
#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/alliance.h"
#include "kernel/building.h"
#include "kernel/build.h"
#include "kernel/curse.h"
#include "kernel/unit.h"
#include "kernel/terrain.h"
#include "kernel/messages.h"
#include "kernel/plane.h"
#include "kernel/region.h"
#include "kernel/save.h"
#include "kernel/ship.h"
#include "kernel/skill.h"
#include "kernel/spell.h"
#include "kernel/item.h"
#include "kernel/faction.h"
#include "kernel/spellbook.h"
#include "races/races.h"

#include "bind_unit.h"
#include "bind_storage.h"
#include "bind_message.h"
#include "bind_building.h"
#include "bind_faction.h"
#include "bind_order.h"
#include "bind_ship.h"
#include "bind_gmtool.h"
#include "bind_region.h"

#include <modules/score.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/order_parser.h>        // for OP_Parser, OrderParserStruct
#include <util/rand.h>
#include <util/rng.h>

#include <selist.h>
#include <stb_ds.h>

#include <dictionary.h>
#include <tolua.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOLUA_PKG(NAME) void tolua_##NAME##_open(lua_State * L)

TOLUA_PKG(eressea);
TOLUA_PKG(process);
TOLUA_PKG(settings);
TOLUA_PKG(config);
TOLUA_PKG(locale);
TOLUA_PKG(log);
TOLUA_PKG(game);

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

int log_lua_error(lua_State * L)
{
    const char *error = lua_tostring(L, -1);
    log_fatal("Lua call failed.\n%s\n", error);
    lua_pop(L, 1);
    return 1;
}

static int tolua_selist_iter(lua_State * L)
{
    selist **qlp = (selist **)lua_touserdata(L, lua_upvalueindex(1));
    selist *ql = *qlp;
    if (ql != NULL) {
        int index = (int)lua_tointeger(L, lua_upvalueindex(2));
        const char *type = lua_tostring(L, lua_upvalueindex(3));
        void *data = selist_get(ql, index);
        tolua_pushusertype(L, data, type);
        selist_advance(qlp, &index, 1);
        lua_pushinteger(L, index);
        lua_replace(L, lua_upvalueindex(2));
        return 1;
    }
    return 0;
}

int tolua_selist_push(struct lua_State *L, const char *list_type,
    const char *elem_type, struct selist *list)
{
    if (list) {
        selist **qlist_ptr =
            (selist **)lua_newuserdata(L, sizeof(selist *));
        *qlist_ptr = list;
        luaL_getmetatable(L, list_type);
        lua_setmetatable(L, -2);
        lua_pushinteger(L, 0);
        lua_pushstring(L, elem_type);
        lua_pushcclosure(L, tolua_selist_iter, 3);       /* OBS: this closure has multiple upvalues (list, index, type_name) */
    }
    else {
        lua_pushnil(L);
    }
    return 1;
}

int tolua_itemlist_next(lua_State * L)
{
    item **item_ptr = (item **)lua_touserdata(L, lua_upvalueindex(1));
    item *itm = *item_ptr;
    if (itm != NULL) {
        tolua_pushstring(L, itm->type->rtype->_name);
        *item_ptr = itm->next;
        return 1;
    }
    return 0;
}

static int tolua_translate(lua_State * L)
{
    const char *str = tolua_tostring(L, 1, 0);
    const char *lang = tolua_tostring(L, 2, 0);
    struct locale *loc = lang ? get_locale(lang) : default_locale;
    if (loc) {
        str = LOC(loc, str);
        tolua_pushstring(L, str);
        return 1;
    }
    return 0;
}

static int tolua_random(lua_State * L)
{
    lua_pushinteger(L, rng_int());
    return 1;
}

static int tolua_message_unit(lua_State * L)
{
    unit *sender = (unit *)tolua_tousertype(L, 1, NULL);
    unit *target = (unit *)tolua_tousertype(L, 2, NULL);
    const char *str = tolua_tostring(L, 3, 0);
    if (!target) {
        tolua_error(L, "target is nil", NULL);
    }
    else if (!sender) {
        tolua_error(L, "sender is nil", NULL);
    }
    else {
        deliverMail(target->faction, sender->region, sender, str, target);
    }
    return 0;
}

static int tolua_message_faction(lua_State * L)
{
    unit *sender = (unit *)tolua_tousertype(L, 1, NULL);
    faction *target = (faction *)tolua_tousertype(L, 2, NULL);
    const char *str = tolua_tostring(L, 3, 0);
    if (!target) {
        tolua_error(L, "target is nil", NULL);
    }
    else if (!sender) {
        tolua_error(L, "sender is nil", NULL);
    }
    else {
        deliverMail(target, sender->region, sender, str, NULL);
    }
    return 0;
}

static int tolua_message_region(lua_State * L)
{
    unit *sender = (unit *)tolua_tousertype(L, 1, NULL);
    const char *str = tolua_tostring(L, 2, 0);
    if (!sender) {
        tolua_error(L, "sender is nil", NULL);
    }
    else {
        ADDMSG(&sender->region->msgs, msg_message("mail_result", "unit message",
            sender, str));
    }
    return 0;
}

static int tolua_update_guards(lua_State * L)
{
    UNUSED_ARG(L);
    update_guards();
    return 0;
}

static int tolua_set_turn(lua_State * L)
{
    turn = (int)tolua_tonumber(L, 1, 0);
    return 0;
}

static int tolua_get_turn(lua_State * L)
{
    lua_pushinteger(L, turn);
    return 1;
}

static int tolua_atoi36(lua_State * L)
{
    const char *s = tolua_tostring(L, 1, 0);
    lua_pushinteger(L, atoi36(s));
    return 1;
}

static int tolua_itoa36(lua_State * L)
{
    int i = (int)tolua_tonumber(L, 1, 0);
    tolua_pushstring(L, itoa36(i));
    return 1;
}

static int tolua_dice_rand(lua_State * L)
{
    const char *s = tolua_tostring(L, 1, 0);
    lua_pushinteger(L, dice_rand(s));
    return 1;
}

static int tolua_get_season(lua_State * L)
{
    int turn_no = (int)tolua_tonumber(L, 1, turn);
    season_t season = calendar_season(turn_no);
    tolua_pushstring(L, seasonnames[season]);
    return 1;
}

static int tolua_create_curse(lua_State * L)
{
    unit *u = (unit *)tolua_tousertype(L, 1, NULL);
    tolua_Error tolua_err;
    attrib **ap = NULL;
    if (tolua_isusertype(L, 2, "unit", 0, &tolua_err)) {
        unit *target = (unit *)tolua_tousertype(L, 2, NULL);
        if (target)
            ap = &target->attribs;
    }
    else if (tolua_isusertype(L, 2, "region", 0, &tolua_err)) {
        region *target = (region *)tolua_tousertype(L, 2, NULL);
        if (target)
            ap = &target->attribs;
    }
    else if (tolua_isusertype(L, 2, "ship", 0, &tolua_err)) {
        ship *target = (ship *)tolua_tousertype(L, 2, NULL);
        if (target)
            ap = &target->attribs;
    }
    else if (tolua_isusertype(L, 2, "building", 0, &tolua_err)) {
        building *target = (building *)tolua_tousertype(L, 2, NULL);
        if (target)
            ap = &target->attribs;
    }
    if (ap) {
        const char *cname = tolua_tostring(L, 3, 0);
        const curse_type *ctype = ct_find(cname);
        if (ctype) {
            float vigour = (float)tolua_tonumber(L, 4, 0);
            int duration = (int)tolua_tonumber(L, 5, 0);
            float effect = (float)tolua_tonumber(L, 6, 0);
            int men = (int)tolua_tonumber(L, 7, 0);   /* optional */
            curse *c = create_curse(u, ap, ctype, vigour, duration, effect, men);
            if (c) {
                tolua_pushboolean(L, true);
                return 1;
            }
        }
    }
    tolua_pushboolean(L, false);
    return 1;
}

static int tolua_update_scores(lua_State * L)
{
    UNUSED_ARG(L);
    score();
    return 0;
}

static int tolua_update_owners(lua_State * L)
{
    region *r;
    UNUSED_ARG(L);
    for (r = regions; r; r = r->next) {
        update_region_owners(r);
    }
    return 0;
}

static int tolua_remove_empty_units(lua_State * L)
{
    UNUSED_ARG(L);
    remove_empty_units();
    return 0;
}

static int tolua_get_nmrs(lua_State * L)
{
    int result = -1;
    int n = (int)tolua_tonumber(L, 1, 0);
    if (n >= 0 && n <= NMRTimeout()) {
        if (nmrs == NULL) {
            update_nmrs(turn);
        }
        result = nmrs[n];
    }
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_spawn_braineaters(lua_State * L)
{
    float chance = (float)tolua_tonumber(L, 1, 0);
    spawn_braineaters(chance);
    return 0;
}

static int tolua_init_reports(lua_State * L)
{
    int result = init_reports();
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_write_report(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    if (f) {
        int options = (int)tolua_tonumber(L, 2, f->options);
        int result = write_reports(f, options, NULL);
        lua_pushinteger(L, result);
    }
    else {
        tolua_pushstring(L, "function expects a faction, got nil");
    }
    return 1;
}

static int tolua_write_reports(lua_State * L)
{
    int result;
    init_reports();
    result = reports();
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_turn_begin(lua_State * L)
{
    faction *f;
    UNUSED_ARG(L);
    for (f = factions; f; f = f->next) {
        if (f->msgs) {
            free_messagelist(f->msgs->begin);
            free(f->msgs);
            f->msgs = NULL;
        }
    }
    turn_begin();
    return 0;
}

static int tolua_turn_process(lua_State * L)
{
    UNUSED_ARG(L);
    turn_process();
    return 0;
}

static int tolua_turn_end(lua_State * L)
{
    UNUSED_ARG(L);
    turn_end();
    return 0;
}

static int tolua_process_orders(lua_State * L)
{
    UNUSED_ARG(L);
    tolua_turn_begin(L);
    tolua_turn_process(L);
    return tolua_turn_end(L);
}

static int tolua_write_passwords(lua_State * L)
{
    int result = writepasswd();
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_write_database(lua_State * L)
{
    int result = gamedb_update();
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_write_summary(lua_State * L)
{
    struct summary *sum;
    UNUSED_ARG(L);
    
    sum = make_summary();
    report_summary(sum, false);
    report_summary(sum, true);
    free_summary(sum);
    return 0;
}
/*
static int tolua_write_map(lua_State * L)
{
    const char *filename = tolua_tostring(L, 1, 0);
    if (filename) {
        crwritemap(filename);
    }
    return 0;
}
*/
static int tolua_read_turn(lua_State * L)
{
    int cturn = current_turn();
    lua_pushinteger(L, cturn);
    return 1;
}

static int tolua_get_region(lua_State * L)
{
    int x = (int)tolua_tonumber(L, 1, 0);
    int y = (int)tolua_tonumber(L, 2, 0);
    region *r;
    pnormalize(&x, &y, findplane(x, y));
    r = findregion(x, y);
    tolua_pushusertype(L, r, "region");
    return 1;
}

static int tolua_get_region_byid(lua_State * L)
{
    int uid = (int)tolua_tonumber(L, 1, 0);
    region *r = findregionbyid(uid);
    tolua_pushusertype(L, r, "region");
    return 1;
}

static int tolua_get_building(lua_State * L)
{
    int no = tolua_toid(L, 1, 0);
    building *b = findbuilding(no);
    tolua_pushusertype(L, b, "building");
    return 1;
}

static int tolua_get_ship(lua_State * L)
{
    int no = tolua_toid(L, 1, 0);
    ship *sh = findship(no);
    tolua_pushusertype(L, sh, "ship");
    return 1;
}

static int tolua_get_alliance(lua_State * L)
{
    int no = tolua_toid(L, 1, 0);
    alliance *f = findalliance(no);
    tolua_pushusertype(L, f, "alliance");
    return 1;
}

static int tolua_get_unit(lua_State * L)
{
    int no = tolua_toid(L, 1, 0);
    unit *u = findunit(no);
    tolua_pushusertype(L, u, "unit");
    return 1;
}

static int tolua_alliance_create(lua_State * L)
{
    int id = (int)tolua_tonumber(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
    alliance *alli = makealliance(id, name);
    tolua_pushusertype(L, alli, "alliance");
    return 1;
}

static int tolua_get_regions(lua_State * L)
{
    region **region_ptr = (region **)lua_newuserdata(L, sizeof(region *));
    luaL_getmetatable(L, "region");
    lua_setmetatable(L, -2);
    *region_ptr = regions;
    lua_pushcclosure(L, tolua_regionlist_next, 1);
    return 1;
}

static int tolua_get_factions(lua_State * L)
{
    faction **faction_ptr = (faction **)lua_newuserdata(L, sizeof(faction *));
    luaL_getmetatable(L, "faction");
    lua_setmetatable(L, -2);
    *faction_ptr = factions;
    lua_pushcclosure(L, tolua_factionlist_next, 1);
    return 1;
}

static int tolua_get_alliance_factions(lua_State * L)
{
    alliance *self = (alliance *)tolua_tousertype(L, 1, NULL);
    return tolua_selist_push(L, "faction_list", "faction", self->members);
}

static int tolua_get_alliance_id(lua_State * L)
{
    alliance *self = (alliance *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->id);
    return 1;
}

static int tolua_get_alliance_name(lua_State * L)
{
    alliance *self = (alliance *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, self->name);
    return 1;
}

static int tolua_set_alliance_name(lua_State * L)
{
    alliance *self = (alliance *)tolua_tousertype(L, 1, NULL);
    alliance_setname(self, tolua_tostring(L, 2, 0));
    return 0;
}

static int config_get_ships(lua_State * L)
{
    selist *ql;
    int qi, i = 0;
    lua_createtable(L, selist_length(shiptypes), 0);
    for (qi = 0, ql = shiptypes; ql; selist_advance(&ql, &qi, 1)) {
        ship_type *stype = (ship_type *)selist_get(ql, qi);
        tolua_pushstring(L, stype->_name);
        lua_rawseti(L, -2, ++i);
    }
    return 1;
}

static int config_get_buildings(lua_State * L)
{
    selist *ql;
    int qi, i = 0;
    lua_createtable(L, selist_length(buildingtypes), 0);
    for (qi = 0, ql = buildingtypes; ql; selist_advance(&ql, &qi, 1)) {
        building_type *btype = (building_type *)selist_get(ql, qi);
        tolua_pushstring(L, btype->_name);
        lua_rawseti(L, -2, ++i);
    }
    return 1;
}

static int config_get_locales(lua_State * L)
{
    const struct locale *lang;
    int i = 0, n = 0;
    for (lang = locales; lang; lang = nextlocale(lang)) ++n;
    lua_createtable(L, n, 0);
    for (lang = locales; lang; lang = nextlocale(lang)) {
        tolua_pushstring(L, locale_name(lang));
        lua_rawseti(L, -2, ++i);
    }
    return 1;
}

static int config_get_resource(lua_State * L)
{
    const char *name = tolua_tostring(L, 1, 0);
    if (name) {
        const struct item_type *itype = it_find(name);
        if (itype) {
            lua_newtable(L);
            lua_pushstring(L, "weight");
            lua_pushinteger(L, itype->weight);
            lua_settable(L, -3);
            if (itype->capacity > 0) {
                lua_pushstring(L, "capacity");
                lua_pushinteger(L, itype->capacity);
                lua_settable(L, -3);
            }
            if (itype->construction) {
                lua_pushstring(L, "build_skill_min");
                lua_pushinteger(L, itype->construction->minskill);
                lua_settable(L, -3);
                lua_pushstring(L, "build_skill_name");
                lua_pushstring(L, skillnames[itype->construction->skill]);
                lua_settable(L, -3);
                if (itype->construction->materials) {
                    int i;
                    lua_pushstring(L, "materials");
                    lua_newtable(L);
                    for (i = 0; itype->construction->materials[i].number; ++i) {
                        lua_pushstring(L,
                            itype->construction->materials[i].rtype->_name);
                        lua_pushinteger(L, itype->construction->materials[i].number);
                        lua_settable(L, -3);
                    }
                    lua_settable(L, -3);
                }
            }
            return 1;
        }
    }
    return 0;
}

static int config_get_btype(lua_State * L)
{
    const char *name = tolua_tostring(L, 1, 0);

    if (name) {
        const struct building_type *btype = bt_find(name);
        if (btype) {
            lua_newtable(L);
            lua_pushstring(L, "flags");
            lua_pushinteger(L, btype->flags);
            lua_settable(L, -3);
            if (btype->capacity > 0) {
                lua_pushstring(L, "capacity");
                lua_pushinteger(L, btype->capacity);
                lua_settable(L, -3);
            }
            if (btype->maxcapacity > 0) {
                lua_pushstring(L, "maxcapacity");
                lua_pushinteger(L, btype->maxcapacity);
                lua_settable(L, -3);
            }
            if (btype->maxsize > 0) {
                lua_pushstring(L, "maxsize");
                lua_pushinteger(L, btype->maxsize);
                lua_settable(L, -3);
            }
            if (btype->maintenance) {
                int i;
                lua_pushstring(L, "maintenance");
                lua_newtable(L);
                for (i = 0; btype->maintenance[i].number; ++i) {
                    lua_pushstring(L,
                        btype->maintenance[i].rtype->_name);
                    lua_pushinteger(L, btype->maintenance[i].number);
                    lua_settable(L, -3);
                }
                lua_settable(L, -3);
            }
            return 1;
        }
    }
    return 0;
}

static int config_get_stype(lua_State * L)
{
    const char *name = tolua_tostring(L, 1, 0);

    if (name) {
        const struct ship_type *stype = st_find(name);
        if (stype) {
            lua_newtable(L);
            lua_pushstring(L, "range");
            lua_pushinteger(L, stype->range);
            lua_settable(L, -3);
            lua_pushstring(L, "cabins");
            lua_pushinteger(L, stype->cabins);
            lua_settable(L, -3);
            lua_pushstring(L, "cargo");
            lua_pushinteger(L, stype->cargo);
            lua_settable(L, -3);
            lua_pushstring(L, "cptskill");
            lua_pushinteger(L, stype->cptskill);
            lua_settable(L, -3);
            lua_pushstring(L, "minskill");
            lua_pushinteger(L, stype->minskill);
            lua_settable(L, -3);
            lua_pushstring(L, "sumskill");
            lua_pushinteger(L, stype->sumskill);
            lua_settable(L, -3);
            lua_pushstring(L, "fishing");
            lua_pushinteger(L, stype->fishing);
            lua_settable(L, -3);
            if (stype->coasts) {
                ptrdiff_t c, n = arrlen(stype->coasts);
                lua_pushstring(L, "coasts");
                lua_newtable(L);
                for (c = 0; c != n; ++c) {
                    lua_pushstring(L, stype->coasts[c]->_name);
                    lua_pushinteger(L, 1);
                    lua_settable(L, -3);
                }
            }
            if (stype->construction) {
                lua_pushstring(L, "build_skill_min");
                lua_pushinteger(L, stype->construction->minskill);
                lua_settable(L, -3);
                lua_pushstring(L, "build_skill_name");
                lua_pushstring(L, skillnames[stype->construction->skill]);
                lua_settable(L, -3);
                if (stype->construction->materials) {
                    int i;
                    lua_pushstring(L, "materials");
                    lua_newtable(L);
                    for (i = 0; stype->construction->materials[i].number; ++i) {
                        lua_pushstring(L,
                            stype->construction->materials[i].rtype->_name);
                        lua_pushinteger(L, stype->construction->materials[i].number);
                        lua_settable(L, -3);
                    }
                    lua_settable(L, -3);
                }
            }
            return 1;
        }
    }
    return 0;
}

static int tolua_get_spell_text(lua_State * L)
{
    const struct locale *loc = default_locale;
    spell *self = (spell *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, spell_info(self, loc));
    return 1;
}

static int tolua_get_spell_name(lua_State * L)
{
    spell *self = (spell *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, self->sname);
    return 1;
}

static int tolua_get_spell_entry_name(lua_State * L)
{
    spellbook_entry *self = (spellbook_entry*)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, spellref_name(&self->spref));
    return 1;
}

static int tolua_get_spell_entry_level(lua_State * L)
{
    spellbook_entry *self = (spellbook_entry*)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, self->level);
    return 1;
}

static int tolua_get_spells(lua_State * L)
{
    return tolua_selist_push(L, "spell_list", "spell", spells);
}

static int tolua_equip_newunits(lua_State * L) {
    unit *u = (unit *)tolua_tousertype(L, 1, NULL);
    equip_newunits(u);
    return 0;
}

static int tolua_report_unit(lua_State * L)
{
    char buffer[512];
    unit *u = (unit *)tolua_tousertype(L, 1, NULL);
    faction *f = (faction *)tolua_tousertype(L, 2, NULL);
    bufunit_depr(f, u, seen_unit, buffer, sizeof(buffer));
    tolua_pushstring(L, buffer);
    return 1;
}

static void parse_inifile(lua_State * L, const dictionary * d, const char *section)
{
    int i;
    const char *arg;
    size_t len = strlen(section);

    for (i = 0; d && i != d->n; ++i) {
        const char *key = d->key[i];
        if (strncmp(section, key, len) == 0 && key[len] == ':') {
            const char *str_value = d->val[i];
            char *endp;
            double num_value = strtod(str_value, &endp);
            lua_pushstring(L, key + len + 1);
            if (*endp) {
                tolua_pushstring(L, str_value);
            }
            else {
                lua_pushnumber(L, num_value);
            }
            lua_rawset(L, -3);
        }
    }

    /* special case */
    lua_pushstring(L, "basepath");
    lua_pushstring(L, basepath());
    lua_rawset(L, -3);
    lua_pushstring(L, "reportpath");
    lua_pushstring(L, reportpath());
    lua_rawset(L, -3);
    arg = config_get("config.rules");
    if (arg) {
        lua_pushstring(L, "rules");
        lua_pushstring(L, arg);
        lua_rawset(L, -3);
    }
}

static int lua_rng_default(lua_State *L) {
    UNUSED_ARG(L);
    random_source_inject_constant(0);
    return 0;
}

static int tolua_parse_orders(lua_State* L)
{
    const char* input = tolua_tostring(L, 1, NULL);
    if (input) {
        parser_state state = { NULL };
        OP_Parser parser = parser_create(&state);
        if (parser) {
            int err;
            err = parser_parse(parser, input, strlen(input), true);
            parser_free(parser);
            lua_pushinteger(L, err);
            return 1;
        }
    }
    return 0;
}

int tolua_bindings_open(lua_State * L, const dictionary *inifile)
{
    tolua_open(L);

    tolua_bind_open(L);

    /* register user types */
    tolua_usertype(L, "spellbook");
    tolua_usertype(L, "spell_entry");
    tolua_usertype(L, "spell");
    tolua_usertype(L, "spell_list");
    tolua_usertype(L, "order");
    tolua_usertype(L, "item");
    tolua_usertype(L, "alliance");
    tolua_usertype(L, "event");
    tolua_usertype(L, "faction_list");
    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_function(L, "parse_orders", tolua_parse_orders);
        tolua_module(L, "directions", 1);
        tolua_beginmodule(L, "directions");
        {
            tolua_constant(L, "WEST", D_WEST);
            tolua_constant(L, "EAST", D_EAST);
            tolua_constant(L, "SOUTHWEST", D_SOUTHWEST);
            tolua_constant(L, "SOUTHEAST", D_SOUTHEAST);
            tolua_constant(L, "NORTHWEST", D_NORTHWEST);
            tolua_constant(L, "NORTHEAST", D_NORTHEAST);
        } tolua_endmodule(L);
        tolua_module(L, "rng", 1);
        tolua_beginmodule(L, "rng");
        {
            tolua_function(L, "inject", lua_rng_default);
            tolua_function(L, "random", tolua_random);
        }
        tolua_endmodule(L);
        tolua_function(L, "rng_int", tolua_random);
        tolua_cclass(L, "alliance", "alliance",
            "", NULL);
        tolua_beginmodule(L, "alliance");
        {
            tolua_variable(L, "name", tolua_get_alliance_name,
                tolua_set_alliance_name);
            tolua_variable(L, "id", tolua_get_alliance_id, NULL);
            tolua_variable(L, "factions", &tolua_get_alliance_factions,
                NULL);
            tolua_function(L, "create", tolua_alliance_create);
        } tolua_endmodule(L);
        tolua_cclass(L, "spell", "spell", "",
            NULL);
        tolua_beginmodule(L, "spell");
        {
            tolua_function(L, "__tostring", tolua_get_spell_name);
            tolua_variable(L, "name", tolua_get_spell_name, 0);
            tolua_variable(L, "text", tolua_get_spell_text, 0);
        } tolua_endmodule(L);
        tolua_cclass(L, "spell_entry", "spell_entry", "",
            NULL);
        tolua_beginmodule(L, "spell_entry");
        {
            tolua_function(L, "__tostring", tolua_get_spell_entry_name);
            tolua_variable(L, "name", tolua_get_spell_entry_name, 0);
            tolua_variable(L, "level", tolua_get_spell_entry_level, 0);
        } tolua_endmodule(L);
        tolua_module(L, "report", 1);
        tolua_beginmodule(L, "report");
        {
            tolua_function(L, "report_unit", &tolua_report_unit);
        } tolua_endmodule(L);
        tolua_module(L, "config", 1);
        tolua_beginmodule(L, "config");
        {
            parse_inifile(L, inifile, "lua");
            tolua_variable(L, "locales", &config_get_locales, 0);
            tolua_function(L, "get_resource", &config_get_resource);
            tolua_variable(L, "buildings", &config_get_buildings, 0);
            tolua_function(L, "get_building", &config_get_btype);
            tolua_variable(L, "ships", &config_get_ships, 0);
            tolua_function(L, "get_ship", &config_get_stype);
        } tolua_endmodule(L);
        tolua_function(L, "get_region_by_id", tolua_get_region_byid);
        tolua_function(L, "get_unit", tolua_get_unit);
        tolua_function(L, "get_alliance", tolua_get_alliance);
        tolua_function(L, "get_ship", tolua_get_ship);
        tolua_function(L, "get_building", tolua_get_building);
        tolua_function(L, "get_region", tolua_get_region);
        tolua_function(L, "factions", tolua_get_factions);
        tolua_function(L, "regions", tolua_get_regions);
        tolua_function(L, "read_turn", tolua_read_turn);
        tolua_function(L, "turn_begin", tolua_turn_begin);
        tolua_function(L, "turn_process", tolua_turn_process);
        tolua_function(L, "turn_end", tolua_turn_end);
        tolua_function(L, "process_orders", tolua_process_orders);
        tolua_function(L, "init_reports", tolua_init_reports);
        tolua_function(L, "write_reports", tolua_write_reports);
        tolua_function(L, "write_report", tolua_write_report);
        tolua_function(L, "write_summary", tolua_write_summary);
        tolua_function(L, "write_passwords", tolua_write_passwords);
        tolua_function(L, "write_database", tolua_write_database);
        tolua_function(L, "message_unit", tolua_message_unit);
        tolua_function(L, "message_faction", tolua_message_faction);
        tolua_function(L, "message_region", tolua_message_region);

        /* scripted monsters */
        tolua_function(L, "spawn_braineaters", tolua_spawn_braineaters);

        tolua_function(L, "update_guards", tolua_update_guards);
        tolua_function(L, "set_turn", &tolua_set_turn);
        tolua_function(L, "get_turn", &tolua_get_turn);
        tolua_function(L, "get_season", tolua_get_season);
        tolua_function(L, "atoi36", tolua_atoi36);
        tolua_function(L, "itoa36", tolua_itoa36);
        tolua_function(L, "dice_roll", tolua_dice_rand);
        tolua_function(L, "get_nmrs", tolua_get_nmrs);
        tolua_function(L, "remove_empty_units", tolua_remove_empty_units);
        tolua_function(L, "update_scores", tolua_update_scores);
        tolua_function(L, "update_owners", tolua_update_owners);
        tolua_function(L, "create_curse", tolua_create_curse);
        tolua_function(L, "translate", &tolua_translate);
        tolua_function(L, "spells", tolua_get_spells);
        tolua_function(L, "equip_newunits", tolua_equip_newunits);
    } tolua_endmodule(L);
    return 1;
}

static void openlibs(lua_State * L)
{
    luaL_openlibs(L);
}

void lua_done(lua_State * L) {
    lua_close(L);
}

lua_State *lua_init(const dictionary *inifile) {
    lua_State *L = luaL_newstate();

    openlibs(L);
    register_tolua_helpers();
    tolua_bindings_open(L, inifile);
    tolua_eressea_open(L);
    tolua_unit_open(L);
    tolua_building_open(L);
    tolua_ship_open(L);
    tolua_region_open(L);
    tolua_faction_open(L);
    tolua_unit_open(L);
    tolua_message_open(L);
    tolua_order_open(L);
#ifdef USE_CURSES
    tolua_gmtool_open(L);
#endif
    tolua_storage_open(L);
    return L;
}

static int run_script(lua_State *L, const char *luafile) {
    int err;
    FILE *F;

    F = fopen(luafile, "r");
    if (!F) {
        log_debug("dofile('%s'): %s", luafile, strerror(errno));
        return errno;
    }
    fclose(F);

    log_debug("executing script %s", luafile);
    lua_getglobal(L, "dofile");
    lua_pushstring(L, luafile);
    err = lua_pcall(L, 1, 1, -3); /* error handler (debug.traceback) is now at stack -3 */
    if (err != 0) {
        log_lua_error(L);
        assert(!"Lua syntax error? check log.");
    }
    else {
        if (lua_isnumber(L, -1)) {
            err = (int)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
    }
    return err;
}

int eressea_run(lua_State *L, const char *luafile)
{
    int err;
    global.vm_state = L;
    /* push an error handling function on the stack: */
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);

    /* try to run configuration scripts: */
    err = run_script(L, "custom.lua");

    /* run the main script */
    if (luafile) {
        err = run_script(L, luafile);
    }
    else {
        err = lua_console(L);
    }
    /* pop error handler off the stack: */
    lua_pop(L, 1);
    return err;
}
