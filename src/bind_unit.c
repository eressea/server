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

#include "bind_unit.h"
#include "alchemy.h"
#include "bindings.h"
#include "move.h"
#include "reports.h"
#include "guard.h"

/*  attributes includes */
#include <attributes/racename.h>
#include <attributes/key.h>

/*  kernel includes */
#include <kernel/building.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/spellbook.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

/*  util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/lists.h>
#include <util/log.h>
#include <selist.h>

#include <tolua.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static int tolua_bufunit(lua_State * L) {
    char buf[8192];
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    int mode = (int)tolua_tonumber(L, 2, (int)seen_unit);
    if (!self)  return 0;

    bufunit(self->faction, self, 0, mode, buf, sizeof(buf));
    tolua_pushstring(L, buf);
    return 1;

}

int tolua_unitlist_nextf(lua_State * L)
{
    unit **unit_ptr = (unit **)lua_touserdata(L, lua_upvalueindex(1));
    unit *u = *unit_ptr;
    if (u != NULL) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");
        *unit_ptr = u->nextF;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

int tolua_unitlist_nextb(lua_State * L)
{
    unit **unit_ptr = (unit **)lua_touserdata(L, lua_upvalueindex(1));
    unit *u = *unit_ptr;
    if (u != NULL) {
        unit *unext = u->next;
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");

        while (unext && unext->building != u->building) {
            unext = unext->next;
        }
        *unit_ptr = unext;

        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

int tolua_unitlist_nexts(lua_State * L)
{
    unit **unit_ptr = (unit **)lua_touserdata(L, lua_upvalueindex(1));
    unit *u = *unit_ptr;
    if (u != NULL) {
        unit *unext = u->next;
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");

        while (unext && unext->ship != u->ship) {
            unext = unext->next;
        }
        *unit_ptr = unext;

        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

int tolua_unitlist_next(lua_State * L)
{
    unit **unit_ptr = (unit **)lua_touserdata(L, lua_upvalueindex(1));
    unit *u = *unit_ptr;
    if (u != NULL) {
        tolua_pushusertype(L, (void *)u, TOLUA_CAST "unit");
        *unit_ptr = u->next;
        return 1;
    }
    else
        return 0;                   /* no more values to return */
}

static int tolua_unit_get_group(lua_State * L)
{
    unit *u = (unit *)tolua_tousertype(L, 1, 0);
    group *g = get_group(u);
    if (g) {
        tolua_pushstring(L, g->name);
        return 1;
    }
    return 0;
}

static int tolua_unit_set_group(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    group *g = join_group(self, tolua_tostring(L, 2, 0));
    lua_pushboolean(L, g!=NULL);
    return 1;
}

static int tolua_unit_get_name(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, unit_getname(self));
    return 1;
}

static int tolua_unit_set_name(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    unit_setname(self, tolua_tostring(L, 2, 0));
    return 0;
}

static int tolua_unit_get_info(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, unit_getinfo(self));
    return 1;
}

static int tolua_unit_set_info(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    unit_setinfo(self, tolua_tostring(L, 2, 0));
    return 0;
}

static int tolua_unit_get_id(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, unit_getid(self));
    return 1;
}

static int tolua_unit_set_id(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    unit_setid(self, (int)tolua_tonumber(L, 2, 0));
    return 0;
}

static int tolua_unit_get_hpmax(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, unit_max_hp(self));
    return 1;
}

static int tolua_unit_get_hp(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, unit_gethp(self));
    return 1;
}

static int tolua_unit_set_hp(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    unit_sethp(self, (int)tolua_tonumber(L, 2, 0));
    return 0;
}

static int tolua_unit_get_number(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, self->number);
    return 1;
}

static int tolua_unit_set_number(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    int number = (int)tolua_tonumber(L, 2, 0);
    if (self->number == 0) {
        set_number(self, number);
        self->hp = unit_max_hp(self) * number;
    }
    else {
        scale_number(self, number);
    }
    return 0;
}

static int tolua_unit_get_flags(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, self->flags);
    return 1;
}

static int tolua_unit_set_flags(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    self->flags = (int)tolua_tonumber(L, 2, 0);
    return 0;
}

static int tolua_unit_get_guard(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushboolean(L, is_guard(self));
    return 1;
}

static int tolua_unit_set_guard(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    unsigned int flags = (unsigned int)tolua_tonumber(L, 2, 0);
    setguard(self, flags!=0);
    return 0;
}

static const char *unit_getmagic(const unit * u)
{
    sc_mage *mage = get_mage(u);
    return mage ? magic_school[mage->magietyp] : NULL;
}

static int tolua_unit_get_magic(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushstring(L, unit_getmagic(self));
    return 1;
}

static void unit_setmagic(unit * u, const char *type)
{
    sc_mage *mage = get_mage(u);
    int mtype;
    for (mtype = 0; mtype != MAXMAGIETYP; ++mtype) {
        if (strcmp(magic_school[mtype], type) == 0)
            break;
    }
    if (mtype == MAXMAGIETYP)
        return;
    if (mage == NULL) {
        mage = create_mage(u, (magic_t)mtype);
    }
}

static int tolua_unit_set_magic(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *type = tolua_tostring(L, 2, 0);
    unit_setmagic(self, type);
    return 0;
}

static int tolua_unit_get_aura(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, get_spellpoints(self));
    return 1;
}

static int tolua_unit_set_aura(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    set_spellpoints(self, (int)tolua_tonumber(L, 2, 0));
    return 0;
}

static int tolua_unit_get_age(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, self->age);
    return 1;
}

static int tolua_unit_set_age(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    self->age = (short)tolua_tonumber(L, 2, 0);
    return 0;
}

static int tolua_unit_get_status(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, unit_getstatus(self));
    return 1;
}

static int tolua_unit_set_status(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    unit_setstatus(self, (status_t)tolua_tonumber(L, 2, 0));
    return 0;
}

static int tolua_unit_get_item(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *iname = tolua_tostring(L, 2, 0);
    int result = -1;

    if (iname != NULL) {
        const item_type *itype = it_find(iname);
        if (itype != NULL) {
            result = i_get(self->items, itype);
        }
    }
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_unit_get_effect(lua_State * L)
{
    const unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *potion_name = tolua_tostring(L, 2, 0);
    int result = -1;
    const potion_type *pt_potion;
    const item_type *it_potion = it_find(potion_name);

    if (it_potion != NULL) {
        pt_potion = it_potion->rtype->ptype;
        if (pt_potion != NULL)
            result = get_effect(self, pt_potion);
    }

    lua_pushinteger(L, result);
    return 1;
}

static int tolua_unit_add_item(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *iname = tolua_tostring(L, 2, 0);
    int number = (int)tolua_tonumber(L, 3, 0);
    int result = -1;

    if (iname != NULL) {
        const item_type *itype = it_find(iname);
        if (itype != NULL) {
            item *i = i_change(&self->items, itype, number);
            result = i ? i->number : 0;
        }
    }
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_unit_getskill(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *skname = tolua_tostring(L, 2, 0);
    skill_t sk = findskill(skname);
    int value = -1;
    if (sk != NOSKILL) {
        skill *sv = unit_skill(self, sk);
        if (sv) {
            value = sv->level;
        }
        else
            value = 0;
    }
    lua_pushinteger(L, value);
    return 1;
}

static int tolua_unit_effskill(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *skname = tolua_tostring(L, 2, 0);
    skill_t sk = findskill(skname);
    int value = (sk == NOSKILL) ? -1 : effskill(self, sk, 0);
    lua_pushinteger(L, value);
    return 1;
}

typedef struct fctr_data {
    unit *target;
    int fhandle;
} fctr_data;

typedef struct event {
    struct event_arg *args;
    char *msg;
} event;

int fctr_handle(struct trigger *tp, void *data)
{
    trigger *t = tp;
    event evt = { 0 };
    fctr_data *fd = (fctr_data *)t->data.v;
    lua_State *L = (lua_State *)global.vm_state;
    unit *u = fd->target;

    evt.args = (event_arg *)data;
    lua_rawgeti(L, LUA_REGISTRYINDEX, fd->fhandle);
    tolua_pushusertype(L, u, TOLUA_CAST "unit");
    tolua_pushusertype(L, &evt, TOLUA_CAST "event");
    if (lua_pcall(L, 2, 0, 0) != 0) {
        const char *error = lua_tostring(L, -1);
        log_error("event (%s): %s\n", unitname(u), error);
        lua_pop(L, 1);
        tolua_error(L, TOLUA_CAST "event handler call failed", NULL);
    }

    return 0;
}

static void fctr_init(trigger * t)
{
    t->data.v = calloc(sizeof(fctr_data), 1);
}

static void fctr_done(trigger * t)
{
    fctr_data *fd = (fctr_data *)t->data.v;
    lua_State *L = (lua_State *)global.vm_state;
    luaL_unref(L, LUA_REGISTRYINDEX, fd->fhandle);
    free(fd);
}

static struct trigger_type tt_lua = {
    "lua_event",
    fctr_init,
    fctr_done,
    fctr_handle
};

static trigger *trigger_lua(struct unit *u, int handle)
{
    trigger *t = t_new(&tt_lua);
    fctr_data *td = (fctr_data *)t->data.v;
    td->target = u;
    td->fhandle = handle;
    return t;
}

static int tolua_unit_addhandler(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *ename = tolua_tostring(L, 2, 0);
    int handle;

    lua_pushvalue(L, 3);
    handle = luaL_ref(L, LUA_REGISTRYINDEX);
    add_trigger(&self->attribs, ename, trigger_lua(self, handle));

    return 0;
}

static int tolua_unit_addnotice(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *str = tolua_tostring(L, 2, 0);

    addmessage(self->region, self->faction, str, MSG_MESSAGE, ML_IMPORTANT);
    return 0;
}

static void unit_castspell(unit * u, const char *name, int level)
{
    spell *sp = find_spell(name);

    if (sp) {
        spellbook *book = unit_get_spellbook(u);
        if (spellbook_get(book, sp)) {
            if (!sp->cast) {
                log_error("spell '%s' has no function.\n", sp->sname);
            }
            else {
                castorder co;
                create_castorder(&co, u, 0, sp, u->region, level, (double)level, 0, 0, 0);
                sp->cast(&co);
                free_castorder(&co);
            }
        }
    }
}

static int tolua_unit_castspell(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *str = tolua_tostring(L, 2, 0);
    int level = (int)tolua_tonumber(L, 3, 1);

    unit_castspell(self, str, level);
    return 0;
}

static int tolua_unit_addspell(lua_State * L)
{
    unit *u = (unit *)tolua_tousertype(L, 1, 0);
    const char *str = tolua_tostring(L, 2, 0);
    int level = (int)tolua_tonumber(L, 3, 1);
    int err = 0;
    spell *sp = find_spell(str);

    if (!sp) {
        log_warning("spell %s could not be found\n", str);
        return EINVAL;
    }
    else {
        unit_add_spell(u, 0, sp, level);
    }

    lua_pushinteger(L, err);
    return 1;
}

static int tolua_unit_set_racename(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *str = tolua_tostring(L, 2, 0);

    set_racename(&self->attribs, str);
    return 0;
}

static int tolua_unit_get_racename(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    attrib *a = a_find(self->attribs, &at_racename);
    if (a) {
        tolua_pushstring(L, get_racename(a));
        return 1;
    }
    return 0;
}

static int tolua_unit_setskill(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *skname = tolua_tostring(L, 2, 0);
    int level = (int)tolua_tonumber(L, 3, 0);
    skill_t sk = findskill(skname);
    if (sk != NOSKILL) {
        set_level(self, sk, level);
    }
    else {
        level = -1;
    }
    lua_pushinteger(L, level);
    return 1;
}

static int tolua_unit_use_pooled(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *iname = tolua_tostring(L, 2, 0);
    int number = (int)tolua_tonumber(L, 3, 0);
    const resource_type *rtype = rt_find(iname);
    int result = -1;
    if (rtype != NULL) {
        result = use_pooled(self, rtype, GET_DEFAULT, number);
    }
    lua_pushinteger(L, result);
    return 1;
}

static int tolua_unit_get_pooled(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *iname = tolua_tostring(L, 2, 0);
    const resource_type *rtype = rt_find(iname);
    int result = -1;
    if (rtype != NULL) {
        result = get_pooled(self, rtype, GET_DEFAULT, INT_MAX);
    }
    lua_pushinteger(L, result);
    return 1;
}

static unit *unit_getfamiliar(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_familiar);
    if (a != NULL) {
        return (unit *)a->data.v;
    }
    return NULL;
}

static int tolua_unit_get_familiar(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, unit_getfamiliar(self), TOLUA_CAST "unit");
    return 1;
}

static int tolua_unit_set_familiar(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    create_newfamiliar(self, (unit *)tolua_tousertype(L, 2, 0));
    return 0;
}

static int tolua_unit_get_building(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, self->building, TOLUA_CAST "building");
    return 1;
}

static int tolua_unit_set_building(lua_State * L)
{
    unit *u = (unit *)tolua_tousertype(L, 1, 0);
    if (u->faction) {
        building * b = (building *)tolua_tousertype(L, 2, 0);
        if (b != u->building) {
            leave(u, true);
            if (b) {
                if (u->region != b->region) {
                    move_unit(u, b->region, NULL);
                }
                u_set_building(u, b);
            }
        }
    }
    return 0;
}

static int tolua_unit_get_ship(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, self->ship, TOLUA_CAST "ship");
    return 1;
}

static void unit_setship(unit * u, ship * s)
{
    leave(u, true);
    if (s && u->region != s->region) {
        move_unit(u, s->region, NULL);
    }
    u_set_ship(u, s);
}

static int tolua_unit_set_ship(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    if (self->faction) {
        unit_setship(self, (ship *)tolua_tousertype(L, 2, 0));
    }
    return 0;
}

static int tolua_unit_get_region(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, self->region, TOLUA_CAST "region");
    return 1;
}

static void unit_setregion(unit * u, region * r)
{
    move_unit(u, r, NULL);
}

static int tolua_unit_set_region(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    unit_setregion(self, (region *)tolua_tousertype(L, 2, 0));
    return 0;
}

static int tolua_unit_add_order(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *str = tolua_tostring(L, 2, 0);
    order *ord = parse_order(str, self->faction->locale);
    unit_addorder(self, ord);
    return 0;
}

static int tolua_unit_clear_orders(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    free_orders(&self->orders);
    return 0;
}

static int tolua_unit_get_items(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);

    item **item_ptr = (item **)lua_newuserdata(L, sizeof(item *));

    luaL_getmetatable(L, TOLUA_CAST "item");
    lua_setmetatable(L, -2);

    *item_ptr = self->items;

    lua_pushcclosure(L, tolua_itemlist_next, 1);

    return 1;
}

static int tolua_unit_get_spells(lua_State * L)
{
    unit *self = (unit *) tolua_tousertype(L, 1, 0);
    sc_mage *mage = self ? get_mage(self) : 0;
    spellbook *sb = mage ? mage->spellbook : 0;
    selist *slist = 0;
    if (sb) {
        selist **slist_ptr = &sb->spells;
        slist = *slist_ptr;
    }
    return tolua_selist_push(L, "spellbook", "spell_entry", slist);
}

static int tolua_unit_get_orders(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);

    order **order_ptr = (order **)lua_newuserdata(L, sizeof(order *));

    luaL_getmetatable(L, TOLUA_CAST "order");
    lua_setmetatable(L, -2);

    *order_ptr = self->orders;

    lua_pushcclosure(L, tolua_orderlist_next, 1);

    return 1;
}

static int tolua_unit_get_curse(lua_State *L) {
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
    if (self->attribs) {
        curse * c = get_curse(self->attribs, ct_find(name));
        if (c) {
            lua_pushnumber(L, curse_geteffect(c));
            return 1;
        }
    }
    return 0;
}

static int tolua_unit_has_attrib(lua_State *L) {
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
    attrib * a = self->attribs;
    while (a) {
        if (strcmp(a->type->name, name) == 0) {
            break;
        }
        a = a->nexttype;
    }
    lua_pushboolean(L, a != NULL);
    return 1;
}

static int tolua_unit_get_flag(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
    int flag = atoi36(name);
    lua_pushboolean(L, key_get(self->attribs, flag));
    return 1;
}

static int tolua_unit_set_flag(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *name = tolua_tostring(L, 2, 0);
    int value = (int)tolua_tonumber(L, 3, 0);
    int flag = atoi36(name);
    if (value) {
        key_set(&self->attribs, flag, value);
    }
    else {
        key_unset(&self->attribs, flag);
    }
    return 0;
}

static int tolua_unit_get_weight(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, unit_getweight(self));
    return 1;
}

static int tolua_unit_get_capacity(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushinteger(L, unit_getcapacity(self));
    return 1;
}

static int tolua_unit_get_faction(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushusertype(L, (void *)self->faction, TOLUA_CAST "faction");
    return 1;
}

static int tolua_unit_set_faction(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    faction *f = (faction *)tolua_tousertype(L, 2, 0);
    u_setfaction(self, f);
    return 0;
}

static int tolua_unit_get_race(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    tolua_pushstring(L, u_race(self)->_name);
    return 1;
}

static int tolua_unit_set_race(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    const char *rcname = tolua_tostring(L, 2, 0);
    const race *rc = rc_find(rcname);
    if (rc != NULL) {
        if (self->irace == u_race(self)) {
            self->irace = NULL;
        }
        u_setrace(self, rc);
    }
    return 0;
}

static int tolua_unit_destroy(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    if (self) {
        remove_unit(&self->region->units, self);
    }
    return 0;
}

static int tolua_unit_create(lua_State * L)
{
    faction *f = (faction *)tolua_tousertype(L, 1, 0);
    region *r = (region *)tolua_tousertype(L, 2, 0);
    unit *u;
    const char *rcname = tolua_tostring(L, 4, NULL);
    int num = (int)tolua_tonumber(L, 3, 1);
    const race *rc;

    assert(f && r);
    rc = rcname ? rc_find(rcname) : f->race;
    assert(rc);
    u = create_unit(r, f, num, rc, 0, NULL, NULL);
    tolua_pushusertype(L, u, TOLUA_CAST "unit");
    return 1;
}

static int tolua_unit_tostring(lua_State * L)
{
    unit *self = (unit *)tolua_tousertype(L, 1, 0);
    lua_pushstring(L, unitname(self));
    return 1;
}

static int tolua_event_gettype(lua_State * L)
{
    event *self = (event *)tolua_tousertype(L, 1, 0);
    int index = (int)tolua_tonumber(L, 2, 0);
    lua_pushstring(L, self->args[index].type);
    return 1;
}

static int tolua_event_get(lua_State * L)
{
    struct event *self = (struct event *)tolua_tousertype(L, 1, 0);
    int index = (int)tolua_tonumber(L, 2, 0);

    event_arg *arg = self->args + index;

    if (arg->type) {
        if (strcmp(arg->type, "string") == 0) {
            tolua_pushstring(L, (const char *)arg->data.v);
        }
        else if (strcmp(arg->type, "int") == 0) {
            lua_pushinteger(L, arg->data.i);
        }
        else if (strcmp(arg->type, "float") == 0) {
            lua_pushnumber(L, (lua_Number)arg->data.f);
        }
        else {
            /* this is pretty lazy */
            tolua_pushusertype(L, (void *)arg->data.v, TOLUA_CAST arg->type);
        }
        return 1;
    }
    tolua_error(L, TOLUA_CAST "invalid type argument for event", NULL);
    return 0;
}

void tolua_unit_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "unit");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_cclass(L, TOLUA_CAST "event", TOLUA_CAST "event", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "event");
        {
            tolua_function(L, TOLUA_CAST "get_type", &tolua_event_gettype);
            tolua_function(L, TOLUA_CAST "get", &tolua_event_get);
        }
        tolua_endmodule(L);

        tolua_cclass(L, TOLUA_CAST "unit", TOLUA_CAST "unit", TOLUA_CAST "", NULL);
        tolua_beginmodule(L, TOLUA_CAST "unit");
        {
            tolua_function(L, TOLUA_CAST "__tostring", &tolua_unit_tostring);
            tolua_function(L, TOLUA_CAST "create", &tolua_unit_create);
            tolua_function(L, TOLUA_CAST "destroy", &tolua_unit_destroy);

            tolua_variable(L, TOLUA_CAST "name", &tolua_unit_get_name,
                tolua_unit_set_name);
            tolua_variable(L, TOLUA_CAST "faction", &tolua_unit_get_faction,
                tolua_unit_set_faction);
            tolua_variable(L, TOLUA_CAST "id", tolua_unit_get_id, tolua_unit_set_id);
            tolua_variable(L, TOLUA_CAST "group", tolua_unit_get_group, tolua_unit_set_group);
            tolua_variable(L, TOLUA_CAST "info", tolua_unit_get_info, tolua_unit_set_info);
            tolua_variable(L, TOLUA_CAST "hp", &tolua_unit_get_hp, tolua_unit_set_hp);
            tolua_variable(L, TOLUA_CAST "status", &tolua_unit_get_status,
                tolua_unit_set_status);
            tolua_variable(L, TOLUA_CAST "familiar", &tolua_unit_get_familiar,
                tolua_unit_set_familiar);

            tolua_variable(L, TOLUA_CAST "weight", &tolua_unit_get_weight, 0);
            tolua_variable(L, TOLUA_CAST "capacity", &tolua_unit_get_capacity, 0);

            tolua_function(L, TOLUA_CAST "add_order", &tolua_unit_add_order);
            tolua_function(L, TOLUA_CAST "clear_orders", &tolua_unit_clear_orders);
            tolua_variable(L, TOLUA_CAST "orders", &tolua_unit_get_orders, 0);

            tolua_function(L, TOLUA_CAST "get_curse", &tolua_unit_get_curse);
            tolua_function(L, TOLUA_CAST "has_attrib", &tolua_unit_has_attrib);

            /*  key-attributes for named flags: */
            tolua_function(L, TOLUA_CAST "set_flag", &tolua_unit_set_flag);
            tolua_function(L, TOLUA_CAST "get_flag", &tolua_unit_get_flag);
            tolua_variable(L, TOLUA_CAST "guard", &tolua_unit_get_guard,
                &tolua_unit_set_guard);
            tolua_variable(L, TOLUA_CAST "flags", &tolua_unit_get_flags,
                &tolua_unit_set_flags);
            tolua_variable(L, TOLUA_CAST "age", &tolua_unit_get_age,
                tolua_unit_set_age);

            /*  items: */
            tolua_function(L, TOLUA_CAST "get_item", &tolua_unit_get_item);
            tolua_function(L, TOLUA_CAST "add_item", &tolua_unit_add_item);
            tolua_variable(L, TOLUA_CAST "items", &tolua_unit_get_items, 0);
            tolua_function(L, TOLUA_CAST "get_pooled", &tolua_unit_get_pooled);
            tolua_function(L, TOLUA_CAST "use_pooled", &tolua_unit_use_pooled);

            /* effects */
            tolua_function(L, TOLUA_CAST "get_potion", &tolua_unit_get_effect);

            /*  skills: */
            tolua_function(L, TOLUA_CAST "get_skill", &tolua_unit_getskill);
            tolua_function(L, TOLUA_CAST "eff_skill", &tolua_unit_effskill);
            tolua_function(L, TOLUA_CAST "set_skill", &tolua_unit_setskill);

            tolua_function(L, TOLUA_CAST "add_notice", &tolua_unit_addnotice);

            /*  npc logic: */
            tolua_function(L, TOLUA_CAST "add_handler", &tolua_unit_addhandler);

            tolua_variable(L, TOLUA_CAST "race_name", &tolua_unit_get_racename,
                &tolua_unit_set_racename);
            tolua_function(L, TOLUA_CAST "add_spell", &tolua_unit_addspell);
            tolua_variable(L, TOLUA_CAST "spells", &tolua_unit_get_spells, 0);
            tolua_function(L, TOLUA_CAST "cast_spell", &tolua_unit_castspell);

            tolua_variable(L, TOLUA_CAST "magic", &tolua_unit_get_magic,
                tolua_unit_set_magic);
            tolua_variable(L, TOLUA_CAST "aura", &tolua_unit_get_aura,
                tolua_unit_set_aura);
            tolua_variable(L, TOLUA_CAST "building", &tolua_unit_get_building,
                tolua_unit_set_building);
            tolua_variable(L, TOLUA_CAST "ship", &tolua_unit_get_ship,
                tolua_unit_set_ship);
            tolua_variable(L, TOLUA_CAST "region", &tolua_unit_get_region,
                tolua_unit_set_region);
            tolua_variable(L, TOLUA_CAST "number", &tolua_unit_get_number,
                tolua_unit_set_number);
            tolua_variable(L, TOLUA_CAST "race", &tolua_unit_get_race,
                tolua_unit_set_race);
            tolua_variable(L, TOLUA_CAST "hp_max", &tolua_unit_get_hpmax, 0);

            tolua_function(L, TOLUA_CAST "show", &tolua_bufunit);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
