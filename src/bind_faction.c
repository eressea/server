#ifdef _MSC_VER
#include <platform.h>
#endif

#include "bind_faction.h"
#include "bind_unit.h"
#include "bindings.h"
#include "magic.h"

#include <kernel/ally.h>
#include <kernel/alliance.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include "kernel/types.h"

#include <util/base36.h>
#include <util/language.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <util/password.h>

#include "attributes/key.h"

#include <lua.h>
#include <lauxlib.h>
#include <tolua.h>
#include <string.h>
#include <stdbool.h>          // for bool
#include <stdio.h>            // for puts

struct allies;

int tolua_factionlist_next(lua_State * L)
{
    faction **faction_ptr = (faction **)lua_touserdata(L, lua_upvalueindex(1));
    faction *f = *faction_ptr;
    if (f != NULL) {
        tolua_pushusertype(L, (void *)f, TOLUA_CAST "faction");
        *faction_ptr = f->next;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

static int tolua_faction_get_units(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    unit **unit_ptr = (unit **)lua_newuserdata(L, sizeof(unit *));

    luaL_getmetatable(L, TOLUA_CAST "unit");
    lua_setmetatable(L, -2);

    *unit_ptr = f->units;

    lua_pushcclosure(L, tolua_unitlist_nextf, 1);
    return 1;
}

int tolua_faction_add_item(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *iname = tolua_tostring(L, 2, NULL);
    int number = (int)tolua_tonumber(L, 3, 0);
    int result = -1;

    if (iname != NULL) {
        const resource_type *rtype = rt_find(iname);
        if (rtype && rtype->itype) {
            item *i = i_change(&f->items, rtype->itype, number);
            result = i ? i->number : 0;
        }                           /* if (itype!=NULL) */
    }
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_faction_get_maxheroes(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, max_heroes(f->num_people));
    return 1;
}

static int tolua_faction_get_heroes(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, countheroes(f));
    return 1;
}

static int tolua_faction_get_score(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushnumber(L, (lua_Number)f->score);
    return 1;
}

static int tolua_faction_get_id(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, f->no);
    return 1;
}

static int tolua_faction_set_id(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    int id = (int)tolua_tonumber(L, 2, 0);
    if (findfaction(id) == NULL) {
        renumber_faction(f, id);
        lua_pushboolean(L, 1);
    }
    else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static int tolua_faction_get_magic(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, magic_school[f->magiegebiet]);
    return 1;
}

static int tolua_faction_set_magic(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *type = tolua_tostring(L, 2, NULL);
    int mtype;

    for (mtype = 0; mtype != MAXMAGIETYP; ++mtype) {
        if (strcmp(magic_school[mtype], type) == 0) {
            f->magiegebiet = (magic_t)mtype;
            break;
        }
    }
    return 0;
}

static int tolua_faction_get_age(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, f->age);
    return 1;
}

static int tolua_faction_set_age(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    int age = (int)tolua_tonumber(L, 2, 0);
    f->age = age;
    return 0;
}

static int tolua_faction_get_flags(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, f->flags);
    return 1;
}

static int tolua_faction_set_flags(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    int flags = (int)tolua_tonumber(L, 2, f->flags);
    f->flags = flags;
    return 1;
}

static int tolua_faction_get_options(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, f->options);
    return 1;
}

static int tolua_faction_set_options(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    int options = (int)tolua_tonumber(L, 2, f->options);
    f->options = options;
    return 1;
}

static int tolua_faction_get_lastturn(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, f->lastorders);
    return 1;
}

static int tolua_faction_set_lastturn(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    if (f) {
        f->lastorders = (int)tolua_tonumber(L, 2, f->lastorders);
    }
    return 0;
}

static int tolua_faction_renumber(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    int no = (int)tolua_tonumber(L, 2, 0);

    renumber_faction(f, no);
    return 0;
}

static int tolua_faction_addnotice(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *str = tolua_tostring(L, 2, NULL);

    addmessage(NULL, f, str, MSG_MESSAGE, ML_IMPORTANT);
    return 0;
}

static int tolua_faction_getkey(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    int flag = atoi36(name);
    int value = key_get(f->attribs, flag);
    if (value != 0) {
        lua_pushinteger(L, value);
        return 1;
    }
    return 0;
}

static int tolua_faction_setkey(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    int value = (int)tolua_tonumber(L, 3, 1);
    int flag = atoi36(name);

    if (value) {
        key_set(&f->attribs, flag, value);
    }
    else {
        key_unset(&f->attribs, flag);
    }
    return 0;
}

static int tolua_faction_debug_messages(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    int i = 1;
    if (f->msgs) {
        mlist *ml;
        for (ml = f->msgs->begin; ml; ml = ml->next, ++i) {
            char buf[120];
            nr_render(ml->msg, default_locale, buf, sizeof(buf), NULL);
            puts(buf);
        }
    }
    return 0;
}

static int tolua_faction_get_messages(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);

    if (f->msgs) {
        int i = 1;
        mlist *ml;
        lua_newtable(L);
        for (ml = f->msgs->begin; ml; ml = ml->next, ++i) {
            lua_pushnumber(L, i);
            lua_pushstring(L, ml->msg->type->name);
            lua_rawset(L, -3);
        }
        return 1;
    }
    return 0;
}

static int tolua_faction_count_msg_type(lua_State *L) {
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *str = tolua_tostring(L, 2, NULL);
    int n = 0;
    if (f->msgs) {
        mlist * ml = f->msgs->begin;
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

static int tolua_faction_normalize(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    region *r = (region *)tolua_tousertype(L, 2, NULL);
    if (r) {
        plane *pl = rplane(r);
        int nx = r->x, ny = r->y;
        pnormalize(&nx, &ny, pl);
        adjust_coordinates(f, &nx, &ny, pl);
        lua_pushinteger(L, nx);
        lua_pushinteger(L, ny);
        return 2;
    }
    return 0;
}

static int tolua_faction_set_origin(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    region *r = (region *)tolua_tousertype(L, 2, NULL);
    plane *pl = rplane(r);
    int id = pl ? pl->id : 0;

    faction_setorigin(f, id, r->x - plane_center_x(pl), r->y - plane_center_y(pl));
    return 0;
}

static int tolua_faction_get_origin(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    int x = 0, y = 0;
    faction_getorigin(f, 0, &x, &y);

    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static int tolua_faction_destroy(lua_State * L)
{
    faction **fp, *f = (faction *)tolua_tousertype(L, 1, NULL);
    /* TODO: this loop is slow af, but what can we do? */
    for (fp = &factions; *fp; fp = &(*fp)->next) {
        if (*fp == f) {
            destroyfaction(fp);
            break;
        }
    }
    return 0;
}

static int tolua_faction_get(lua_State * L)
{
    int no = tolua_toid(L, 1, 0);
    faction *f = findfaction(no);
    tolua_pushusertype(L, f, TOLUA_CAST "faction");
    return 1;
}

static int tolua_faction_create(lua_State * L)
{
    const char *racename = tolua_tostring(L, 1, NULL);
    const char *email = tolua_tostring(L, 2, NULL);
    const char *lang = tolua_tostring(L, 3, NULL);
    struct locale *loc = lang ? get_locale(lang) : default_locale;
    faction *f = NULL;
    const struct race *frace = rc_find(racename ? racename : "human");
    if (frace != NULL) {
        f = addfaction(email, NULL, frace, loc);
    }
    if (!f) {
        log_error("cannot create %s faction for %s, unknown race.", racename, email);
    }
    tolua_pushusertype(L, f, TOLUA_CAST "faction");
    return 1;
}

static int tolua_faction_get_password(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, faction_getpassword(f));
    return 1;
}

static int tolua_faction_set_password(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char * passw = tolua_tostring(L, 2, NULL);
    faction_setpassword(f, 
      passw ? password_hash(passw, PASSWORD_DEFAULT) : NULL);
    return 0;
}

static int tolua_faction_get_email(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, faction_getemail(f));
    return 1;
}

static int tolua_faction_set_email(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    faction_setemail(f, tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_faction_get_locale(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, locale_name(f->locale));
    return 1;
}

static int tolua_faction_set_locale(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    const struct locale *loc = get_locale(name);
    if (loc) {
        f->locale = loc;
    }
    else {
        tolua_pushstring(L, "invalid locale");
        return 1;
    }
    return 0;
}

static int tolua_faction_get_race(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, f->race->_name);
    return 1;
}

static int tolua_faction_set_race(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    const char *name = tolua_tostring(L, 2, NULL);
    const race *rc = rc_find(name);
    if (rc != NULL) {
        f->race = rc;
    }

    return 0;
}

static int tolua_faction_get_name(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, faction_getname(f));
    return 1;
}

static int tolua_faction_set_name(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    faction_setname(f, tolua_tostring(L, 2, NULL));
    return 0;
}

static int tolua_faction_get_uid(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushinteger(L, f->uid);
    return 1;
}

static int tolua_faction_set_uid(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    f->uid = (int)tolua_tonumber(L, 2, 0);
    return 0;
}

static int tolua_faction_get_info(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    tolua_pushstring(L, faction_getbanner(f));
    return 1;
}

static int tolua_faction_set_info(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    faction_setbanner(f, tolua_tostring(L, 2, NULL));
    return 0;
}

/* TODO: this is probably useful elsewhere */
static const char *status_names[] = {
    "money", "fight", "observe", "give", "guard", "stealth", "travel", NULL
};

static int cb_ally_push(struct allies *af, struct faction *f, int status, void *udata) {
    struct lua_State *L = (struct lua_State *)udata;
    int len = 1;
    int i;

    lua_pushnumber(L, f->no);
    lua_newtable(L);
    for (i = 0; status_names[i]; ++i) {
        int flag = 1 << i;
        if (status & flag) {
            lua_pushstring(L, status_names[i]);
            lua_rawseti(L, -2, len++);
        }
    }

    lua_rawset(L, -3);
    return 0;
}

static int tolua_faction_get_allies(lua_State * L) {
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_newtable(L);
    allies_walk(f->allies, cb_ally_push, L);
    return 1;
}

static int tolua_faction_set_ally(lua_State * L) {
    faction *f1 = (faction *)tolua_tousertype(L, 1, NULL);
    faction *f2 = (faction *)tolua_tousertype(L, 2, NULL);
    const char *status = tolua_tostring(L, 3, NULL);
    bool value = tolua_toboolean(L, 4, 1);
    if (status) {
        int flag = ally_status(status);
        int flags = ally_get(f1->allies, f2);
        if (value) {
            flags |= flag;
        }
        else {
            flags &= ~flag;
        }
        ally_set(&f1->allies, f2, flags);
    }
    return 0;
}

static int tolua_faction_get_ally(lua_State * L)
{
    faction *f1 = (faction *)tolua_tousertype(L, 1, NULL);
    faction *f2 = (faction *)tolua_tousertype(L, 2, NULL);
    const char *status = tolua_tostring(L, 3, NULL);
    if (f1 && f2 && status) {
        int test = ally_status(status);
        int flags = ally_get(f1->allies, f2);
        lua_pushboolean(L, (test & flags) == test);
        return 1;
    }
    return 0;
}

static int tolua_faction_get_alliances(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    tolua_pushusertype(L, f_get_alliance(f), TOLUA_CAST "alliance");
    return 1;
}

static int tolua_faction_set_alliance(lua_State * L)
{
    struct faction *f = (struct faction *)tolua_tousertype(L, 1, NULL);
    struct alliance *alli = (struct alliance *) tolua_tousertype(L, 2, NULL);

    setalliance(f, alli);

    return 0;
}

static int tolua_faction_get_items(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    item **item_ptr = (item **)lua_newuserdata(L, sizeof(item *));

    luaL_getmetatable(L, TOLUA_CAST "item");
    lua_setmetatable(L, -2);

    *item_ptr = f->items;

    lua_pushcclosure(L, tolua_itemlist_next, 1);

    return 1;
}

static int tolua_faction_tostring(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, NULL);
    lua_pushstring(L, factionname(f));
    return 1;
}

void tolua_faction_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "faction");
    tolua_usertype(L, TOLUA_CAST "faction_list");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_function(L, TOLUA_CAST "get_faction", tolua_faction_get);
        tolua_beginmodule(L, TOLUA_CAST "eressea");
        tolua_function(L, TOLUA_CAST "faction", tolua_faction_get);
        tolua_endmodule(L);
        tolua_cclass(L, TOLUA_CAST "faction", TOLUA_CAST "faction", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "faction");
        {
            tolua_function(L, TOLUA_CAST "__tostring", tolua_faction_tostring);

            tolua_variable(L, TOLUA_CAST "id", tolua_faction_get_id,
                tolua_faction_set_id);
            tolua_variable(L, TOLUA_CAST "uid", tolua_faction_get_uid,
                tolua_faction_set_uid);
            tolua_variable(L, TOLUA_CAST "name", tolua_faction_get_name,
                tolua_faction_set_name);
            tolua_variable(L, TOLUA_CAST "info", tolua_faction_get_info,
                tolua_faction_set_info);
            tolua_variable(L, TOLUA_CAST "units", tolua_faction_get_units, NULL);
            tolua_variable(L, TOLUA_CAST "heroes", tolua_faction_get_heroes, NULL);
            tolua_variable(L, TOLUA_CAST "maxheroes", tolua_faction_get_maxheroes,
                NULL);
            tolua_variable(L, TOLUA_CAST "password", tolua_faction_get_password,
                tolua_faction_set_password);
            tolua_variable(L, TOLUA_CAST "email", tolua_faction_get_email,
                tolua_faction_set_email);
            tolua_variable(L, TOLUA_CAST "locale", tolua_faction_get_locale,
                tolua_faction_set_locale);
            tolua_variable(L, TOLUA_CAST "race", tolua_faction_get_race,
                tolua_faction_set_race);
            tolua_variable(L, TOLUA_CAST "score", tolua_faction_get_score, NULL);
            tolua_variable(L, TOLUA_CAST "magic", tolua_faction_get_magic,
                tolua_faction_set_magic);
            tolua_variable(L, TOLUA_CAST "age", tolua_faction_get_age,
                tolua_faction_set_age);
            tolua_variable(L, TOLUA_CAST "options", tolua_faction_get_options,
                tolua_faction_set_options);
            tolua_variable(L, TOLUA_CAST "flags", tolua_faction_get_flags, tolua_faction_set_flags);
            tolua_variable(L, TOLUA_CAST "lastturn", tolua_faction_get_lastturn,
                tolua_faction_set_lastturn);

            tolua_variable(L, TOLUA_CAST "alliance", tolua_faction_get_alliances,
                tolua_faction_set_alliance);

            tolua_variable(L, TOLUA_CAST "allies", tolua_faction_get_allies, NULL);
            tolua_function(L, TOLUA_CAST "set_ally", tolua_faction_set_ally);
            tolua_function(L, TOLUA_CAST "get_ally", tolua_faction_get_ally);

            tolua_function(L, TOLUA_CAST "get_origin", tolua_faction_get_origin);
            tolua_function(L, TOLUA_CAST "set_origin", tolua_faction_set_origin);
            tolua_function(L, TOLUA_CAST "normalize", tolua_faction_normalize);

            tolua_function(L, TOLUA_CAST "add_item", tolua_faction_add_item);
            tolua_variable(L, TOLUA_CAST "items", tolua_faction_get_items, NULL);

            tolua_function(L, TOLUA_CAST "renumber", tolua_faction_renumber);
            tolua_function(L, TOLUA_CAST "create", tolua_faction_create);
            tolua_function(L, TOLUA_CAST "get", tolua_faction_get);
            tolua_function(L, TOLUA_CAST "destroy", tolua_faction_destroy);
            tolua_function(L, TOLUA_CAST "add_notice", tolua_faction_addnotice);

            /* tech debt hack, siehe https://paper.dropbox.com/doc/Weihnachten-2015-5tOx5r1xsgGDBpb0gILrv#:h=Probleme-mit-Tests-(Nachtrag-0 */
            tolua_function(L, TOLUA_CAST "count_msg_type", tolua_faction_count_msg_type);
            tolua_variable(L, TOLUA_CAST "messages", tolua_faction_get_messages, NULL);
            tolua_function(L, TOLUA_CAST "debug_messages", tolua_faction_debug_messages);

            tolua_function(L, TOLUA_CAST "get_key", tolua_faction_getkey);
            tolua_function(L, TOLUA_CAST "set_key", tolua_faction_setkey);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
