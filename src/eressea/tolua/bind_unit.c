/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <config.h>

#include "bind_unit.h"
#include "bindings.h"

// Atributes includes
#include <attributes/racename.h>
#include <attributes/key.h>

// kernel includes
#include <kernel/building.h>
#include <kernel/eressea.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/move.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

// util includes
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/lists.h>
#include <util/log.h>

#include <lua.h>
#include <tolua++.h>

#include <limits.h>

static int
tolua_unit_get_objects(lua_State* tolua_S)
{
  unit * self = (unit *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, (void*)&self->attribs, "hashtable");
  return 1;
}

int tolua_unitlist_nextf(lua_State *tolua_S)
{
  unit** unit_ptr = (unit **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  unit * u = *unit_ptr;
  if (u != NULL) {
    tolua_pushusertype(tolua_S, (void*)u, "unit");
    *unit_ptr = u->nextF;
    return 1;
  }
  else return 0;  /* no more values to return */
}

int tolua_unitlist_nextb(lua_State *tolua_S)
{
  unit** unit_ptr = (unit **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  unit * u = *unit_ptr;
  if (u != NULL) {
    unit * unext = u->next;
    tolua_pushusertype(tolua_S, (void*)u, "unit");

    while (unext && unext->building!=u->building) {
      unext = unext->next;
    }
    *unit_ptr = unext;

    return 1;
  }
  else return 0;  /* no more values to return */
}

int tolua_unitlist_nexts(lua_State *tolua_S)
{
  unit** unit_ptr = (unit **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  unit * u = *unit_ptr;
  if (u != NULL) {
    unit * unext = u->next;
    tolua_pushusertype(tolua_S, (void*)u, "unit");

    while (unext && unext->ship!=u->ship) {
      unext = unext->next;
    }
    *unit_ptr = unext;

    return 1;
  }
  else return 0;  /* no more values to return */
}

int tolua_unitlist_next(lua_State *tolua_S)
{
  unit** unit_ptr = (unit **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  unit * u = *unit_ptr;
  if (u != NULL) {
    tolua_pushusertype(tolua_S, (void*)u, "unit");
    *unit_ptr = u->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}


static int tolua_unit_get_name(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, self->name);
  return 1;
}

static int tolua_unit_set_name(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_info(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, unit_getinfo(self));
  return 1;
}

static int tolua_unit_set_info(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setinfo(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_id(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)unit_getid(self));
  return 1;
}

static int tolua_unit_set_id(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setid(self, (int)tolua_tonumber(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_hpmax(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)unit_max_hp(self));
  return 1;
}

static int tolua_unit_get_hp(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)unit_gethp(self));
  return 1;
}

static int tolua_unit_set_hp(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_sethp(self, (int)tolua_tonumber(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_number(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->number);
  return 1;
}

static int tolua_unit_set_number(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  int number = (int)tolua_tonumber(tolua_S, 2, 0);
  if (self->number==0) {
    set_number(self, number);
    self->hp = unit_max_hp(self) * number;
  } else {
    scale_number(self, number);
  }
  return 0;
}

static int tolua_unit_get_flags(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->flags);
  return 1;
}

static int tolua_unit_set_flags(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  self->flags = (int)tolua_tonumber(tolua_S, 2, 0);
  return 0;
}

static const char *
unit_getmagic(const unit * u)
{
  sc_mage * mage = get_mage(u);
  return mage?magietypen[mage->magietyp]:NULL;
}

static int tolua_unit_get_magic(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  lua_pushstring(tolua_S, unit_getmagic(self));
  return 1;
}

static void
unit_setmagic(unit * u, const char * type)
{
  sc_mage * mage = get_mage(u);
  magic_t mtype;
  for (mtype=0;mtype!=MAXMAGIETYP;++mtype) {
    if (strcmp(magietypen[mtype], type)==0) break;
  }
  if (mtype==MAXMAGIETYP) return;
  if (mage==NULL) {
    mage = create_mage(u, mtype);
  }
}

static int tolua_unit_set_magic(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * type = tolua_tostring(tolua_S, 2, 0);
  unit_setmagic(self, type);
  return 0;
}

static int tolua_unit_get_aura(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)get_spellpoints(self));
  return 1;
}

static int tolua_unit_set_aura(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  set_spellpoints(self, (int)tolua_tonumber(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_age(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->age);
  return 1;
}

static int tolua_unit_set_age(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  self->age = (short)tolua_tonumber(tolua_S, 2, 0);
  return 0;
}

static int tolua_unit_get_status(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)unit_getstatus(self));
  return 1;
}

static int tolua_unit_set_status(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setstatus(self, (status_t)tolua_tonumber(tolua_S, 2, 0));
  return 0;
}

static int
tolua_unit_get_item(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * iname = tolua_tostring(tolua_S, 2, 0);
  int result = -1;

  if (iname!=NULL) {
    const item_type * itype = it_find(iname);
    if (itype!=NULL) {
      result = i_get(self->items, itype);
    }
  }
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_unit_add_item(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * iname = tolua_tostring(tolua_S, 2, 0);
  int number = (int)tolua_tonumber(tolua_S, 3, 0);
  int result = -1;

  if (iname!=NULL) {
    const item_type * itype = it_find(iname);
    if (itype!=NULL) {
      item * i = i_change(&self->items, itype, number);
      result = i?i->number:0;
    }
  }
  lua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_unit_getskill(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * skname = tolua_tostring(tolua_S, 2, 0);
  skill_t sk = sk_find(skname);
  int value = -1;
  if (sk!=NOSKILL) {
    skill * sv = get_skill(self, sk);
    if (sv) {
      value = sv->level;
    }
    else value = 0;
  }
  lua_pushnumber(tolua_S, (lua_Number)value);
  return 1;
}

static int
tolua_unit_effskill(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * skname = tolua_tostring(tolua_S, 2, 0);
  skill_t sk = sk_find(skname);
  int value = (sk==NOSKILL)?-1:eff_skill(self, sk, self->region);
  lua_pushnumber(tolua_S, (lua_Number)value);
  return 1;
}

typedef struct fctr_data {
  unit * target;
  int fhandle;
} fctr_data;

typedef struct event {
  struct event_arg * args;
  char * msg;
} event;

int
fctr_handle(struct trigger * tp, void * data)
{
  trigger * t = tp;
  event evt = { 0 };
  fctr_data * fd = (fctr_data*)t->data.v;
  lua_State * L = (lua_State *)global.vm_state;
  unit * u = fd->target;
  
  evt.args = (event_arg*)data;
  lua_rawgeti(L, LUA_REGISTRYINDEX, fd->fhandle);
  tolua_pushusertype(L, u, "unit");
  tolua_pushusertype(L, &evt, "event");
  if (lua_pcall(L, 2, 0, 0)!=0) {
    const char* error = lua_tostring(L, -1);
    log_error(("event (%s): %s\n", unitname(u), error));
    lua_pop(L, 1);
    tolua_error(L, "event handler call failed", NULL);
  }

  return 0;
}

static void
fctr_init(trigger * t)
{
  t->data.v = calloc(sizeof(fctr_data), 1);
}

static void
fctr_done(trigger * t)
{
  fctr_data * fd = (fctr_data*)t->data.v;
  lua_State * L = (lua_State *)global.vm_state;
  luaL_unref(L, LUA_REGISTRYINDEX, fd->fhandle);
  free(fd);
}

static struct trigger_type tt_lua = {
  "lua_event",
  fctr_init,
  fctr_done,
  fctr_handle
};

static trigger *
trigger_lua(struct unit * u, int handle)
{
  trigger * t = t_new(&tt_lua);
  fctr_data * td = (fctr_data*)t->data.v;
  td->target = u;
  td->fhandle = handle;
  return t;
}

static int
tolua_unit_addhandler(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * ename = tolua_tostring(tolua_S, 2, 0);
  int handle;

  lua_pushvalue(tolua_S, 3);
  handle = luaL_ref(tolua_S, LUA_REGISTRYINDEX);
  add_trigger(&self->attribs, ename, trigger_lua(self, handle));

  return 0;
}

static int
tolua_unit_addnotice(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * str = tolua_tostring(tolua_S, 2, 0);

  addmessage(self->region, self->faction, str, MSG_MESSAGE, ML_IMPORTANT);
  return 0;
}

static void
unit_castspell(unit * u, const char * name)
{
  spell_list * slist = spells;
  while (slist!=NULL) {
    spell * sp = slist->data;
    if (strcmp(name, sp->sname)==0) {
      castorder * co = (castorder*)malloc(sizeof(castorder));
      co->distance = 0;
      co->familiar = NULL;
      co->force = sp->level;
      co->level = sp->level;
      co->magician.u = u;
      co->order = NULL;
      co->par = NULL;
      co->rt = u->region;
      co->sp = sp;
      if (sp->sp_function==NULL) {
        log_error(("spell '%s' has no function.\n", sp->sname));
        co->level = 0;
      } else {
        sp->sp_function(co);
      }
      free(co);
    }
    slist=slist->next;
  }
}

static int
tolua_unit_castspell(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * str = tolua_tostring(tolua_S, 2, 0);
  unit_castspell(self, str);
  return 0;
}

static void
unit_addspell(unit * u, const char * name)
{
  int add = 0;
  spell_list * slist = spells;
  while (slist!=NULL) {
    spell * sp = slist->data;
    if (strcmp(name, sp->sname)==0) {
      struct sc_mage * mage = get_mage(u);
      if (add) log_error(("two spells are called %s.\n", name));
      add_spell(mage, sp);
      add = 1;
    }
    slist=slist->next;
  }
  if (!add) log_error(("spell %s could not be found\n", name));
}

static int
tolua_unit_addspell(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * str = tolua_tostring(tolua_S, 2, 0);
  unit_addspell(self, str);
  return 0;
}

static void
unit_removespell(unit * u, const spell * sp)
{
  sc_mage * mage = get_mage(u);
  if (mage!=NULL) {
    spell_list ** isptr = &mage->spells;
    while (*isptr && (*isptr)->data != sp) {
      isptr = &(*isptr)->next;
    }
    if (*isptr) {
      spell_list * sptr = *isptr;
      *isptr = sptr->next;
      free(sptr);
    }
  }
}

static int
tolua_unit_removespell(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  spell * sp = (spell*)tolua_tousertype(tolua_S, 2, 0);
  unit_removespell(self, sp);
  return 0;
}

static int
tolua_unit_setracename(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * str = tolua_tostring(tolua_S, 2, 0);

  set_racename(&self->attribs, str);
  return 0;
}

static int
tolua_unit_setskill(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * skname = tolua_tostring(tolua_S, 2, 0);
  int level = (int)tolua_tonumber(tolua_S, 3, 0);
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    set_level(self, sk, level);
  } else {
    level = -1;
  }
  lua_pushnumber(tolua_S, (lua_Number)level);
  return 1;
}

static int
tolua_unit_use_pooled(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * iname = tolua_tostring(tolua_S, 2, 0);
  int number = (int)tolua_tonumber(tolua_S, 3, 0);
  const resource_type * rtype = rt_find(iname);
  int result = -1;
  if (rtype!=NULL) {
    result = use_pooled(self, rtype, GET_DEFAULT, number);
  }
  lua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_unit_get_pooled(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * iname = tolua_tostring(tolua_S, 2, 0);
  const resource_type * rtype = rt_find(iname);
  int result = -1;
  if (rtype!=NULL) {
    result = get_pooled(self, rtype, GET_DEFAULT, INT_MAX);
  }
  lua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static unit *
unit_getfamiliar(const unit * u)
{
  attrib * a = a_find(u->attribs, &at_familiar);
  if (a!=NULL) {
    return (unit*)a->data.v;
  }
  return NULL;
}

static int tolua_unit_get_familiar(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, unit_getfamiliar(self), "unit");
  return 1;
}

static int tolua_unit_set_familiar(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  create_newfamiliar(self, (unit *)tolua_tousertype(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_building(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, self->building, "building");
  return 1;
}

static void
unit_setbuilding(unit * u, building * b)
{
  leave(u->region, u);
  if (u->region!=b->region) {
    move_unit(u, b->region, NULL);
  }
  u->building = b;
}

static int tolua_unit_set_building(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setbuilding(self, (building *)tolua_tousertype(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_ship(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, self->ship, "ship");
  return 1;
}

static void
unit_setship(unit * u, ship * s)
{
  leave(u->region, u);
  if (u->region!=s->region) {
    move_unit(u, s->region, NULL);
  }
  u->ship = s;
}

static int tolua_unit_set_ship(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setship(self, (ship *)tolua_tousertype(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_get_region(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, self->region, "region");
  return 1;
}

static void
unit_setregion(unit * u, region * r)
{
  move_unit(u, r, NULL);
}

static int tolua_unit_set_region(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setregion(self, (region *)tolua_tousertype(tolua_S, 2, 0));
  return 0;
}

static int tolua_unit_add_order(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * str = tolua_tostring(tolua_S, 2, 0);
  order * ord = parse_order(str, self->faction->locale);
  unit_addorder(self, ord);
  return 0;
}

static int tolua_unit_clear_orders(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  free_orders(&self->orders);
  return 0;
}

static int tolua_unit_get_items(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);

  item ** item_ptr = (item **)lua_newuserdata(tolua_S, sizeof(item *));

  luaL_getmetatable(tolua_S, "item");
  lua_setmetatable(tolua_S, -2);

  *item_ptr = self->items;

  lua_pushcclosure(tolua_S, tolua_itemlist_next, 1);

  return 1;
}

static int tolua_unit_get_spells(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  sc_mage * mage;
  spell_list ** spell_ptr = (spell_list **)lua_newuserdata(tolua_S, sizeof(spell_list *));

  luaL_getmetatable(tolua_S, "spell_list");
  lua_setmetatable(tolua_S, -2);

  mage = get_mage(self);

  *spell_ptr = mage?mage->spells:0;

  lua_pushcclosure(tolua_S, tolua_spelllist_next, 1);

  return 1;
}

static int tolua_unit_get_orders(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);

  order ** order_ptr = (order **)lua_newuserdata(tolua_S, sizeof(order *));

  luaL_getmetatable(tolua_S, "order");
  lua_setmetatable(tolua_S, -2);

  *order_ptr = self->orders;

  lua_pushcclosure(tolua_S, tolua_orderlist_next, 1);

  return 1;
}

static int tolua_unit_get_flag(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  int flag = atoi36(name);
  attrib * a = find_key(self->attribs, flag);
  lua_pushboolean(tolua_S, (a!=NULL));
  return 1;
}

static int tolua_unit_set_flag(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  int value = (int)tolua_tonumber(tolua_S, 3, 0);
  int flag = atoi36(name);
  attrib * a = find_key(self->attribs, flag);
  if (a==NULL && value) {
    add_key(&self->attribs, flag);
  } else if (a!=NULL && !value) {
    a_remove(&self->attribs, a);
  }
  return 0;
}

static int tolua_unit_get_weight(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)unit_getweight(self));
  return 1;
}

static int tolua_unit_get_capacity(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)unit_getcapacity(self));
  return 1;
}

static int tolua_unit_get_faction(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, (void*)self->faction, "faction");
  return 1;
}

static int tolua_unit_set_faction(lua_State* tolua_S)
{
  unit * self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  faction * f = (faction *)tolua_tousertype(tolua_S, 2, 0);
  u_setfaction(self, f);
  return 0;
}

static int tolua_unit_get_race(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, self->race->_name[0]);
  return 1;
}

static int tolua_unit_set_race(lua_State* tolua_S)
{
  unit * self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  const char * rcname = tolua_tostring(tolua_S, 2, 0);
  race * rc = rc_find(rcname);
  if (rc!=NULL) {
    if (self->irace==self->race) self->irace = rc;
    self->race = rc;
  }
  return 0;
}

static int
tolua_unit_create(lua_State* tolua_S)
{
  faction * f = (faction *)tolua_tousertype(tolua_S, 1, 0);
  region * r = (region *)tolua_tousertype(tolua_S, 2, 0);
  unit * u = create_unit(r, f, 0, f->race, 0, NULL, NULL);

  tolua_pushusertype(tolua_S, u, "unit");
  return 1;
}

static int
tolua_unit_tostring(lua_State *tolua_S)
{
  unit * self = (unit *)tolua_tousertype(tolua_S, 1, 0);
  lua_pushstring(tolua_S, unitname(self));
  return 1;
}

static int
tolua_event_gettype(lua_State *tolua_S)
{
  event * self = (event *)tolua_tousertype(tolua_S, 1, 0);
  int index = (int)tolua_tonumber(tolua_S, 2, 0);
  lua_pushstring(tolua_S, self->args[index].type);
  return 1;
}

static int
tolua_event_get(lua_State *tolua_S)
{
  struct event * self = (struct event *)tolua_tousertype(tolua_S, 1, 0);
  int index = (int)tolua_tonumber(tolua_S, 2, 0);

  event_arg * arg = self->args+index;

  if (arg->type) {
    if (strcmp(arg->type, "string")==0) {
      tolua_pushstring(tolua_S, (const char *)arg->data.v);
    } else if (strcmp(arg->type, "int")==0) {
      tolua_pushnumber(tolua_S, (lua_Number)arg->data.i);
    } else if (strcmp(arg->type, "float")==0) {
      tolua_pushnumber(tolua_S, (lua_Number)arg->data.f);
    } else {
      /* this is pretty lazy */
      tolua_pushusertype(tolua_S, (void*)arg->data.v, arg->type);
    }
    return 1;
  }
  tolua_error(tolua_S, "invalid type argument for event", NULL);
  return 0;
}

void
tolua_unit_open(lua_State * tolua_S)
{
  /* register user types */
  tolua_usertype(tolua_S, "unit");
  tolua_usertype(tolua_S, "unit_list");

  tolua_module(tolua_S, NULL, 0);
  tolua_beginmodule(tolua_S, NULL);
  {
    tolua_cclass(tolua_S, "event", "event", "", NULL);
    tolua_beginmodule(tolua_S, "event");
    {
      tolua_function(tolua_S, "get_type", tolua_event_gettype);
      tolua_function(tolua_S, "get", tolua_event_get);
    }
    tolua_endmodule(tolua_S);

    tolua_cclass(tolua_S, "unit", "unit", "", NULL);
    tolua_beginmodule(tolua_S, "unit");
    {
      tolua_function(tolua_S, "__tostring", tolua_unit_tostring);
      tolua_function(tolua_S, "create", tolua_unit_create);

      tolua_variable(tolua_S, "name", tolua_unit_get_name, tolua_unit_set_name);
      tolua_variable(tolua_S, "faction", tolua_unit_get_faction, tolua_unit_set_faction);
      tolua_variable(tolua_S, "id", tolua_unit_get_id, tolua_unit_set_id);
      tolua_variable(tolua_S, "info", tolua_unit_get_info, tolua_unit_set_info);
      tolua_variable(tolua_S, "hp", tolua_unit_get_hp, tolua_unit_set_hp);
      tolua_variable(tolua_S, "status", tolua_unit_get_status, tolua_unit_set_status);
      tolua_variable(tolua_S, "familiar", tolua_unit_get_familiar, tolua_unit_set_familiar);

      tolua_variable(tolua_S, "weight", tolua_unit_get_weight, 0);
      tolua_variable(tolua_S, "capacity", tolua_unit_get_capacity, 0);

      tolua_function(tolua_S, "add_order", tolua_unit_add_order);
      tolua_function(tolua_S, "clear_orders", tolua_unit_clear_orders);
      tolua_variable(tolua_S, "orders", tolua_unit_get_orders, 0);

      // key-attributes for named flags:
      tolua_function(tolua_S, "set_flag", tolua_unit_set_flag);
      tolua_function(tolua_S, "get_flag", tolua_unit_get_flag);
      tolua_variable(tolua_S, "flags", tolua_unit_get_flags, tolua_unit_set_flags);
      tolua_variable(tolua_S, "age", tolua_unit_get_age, tolua_unit_set_age);

      // items:
      tolua_function(tolua_S, "get_item", tolua_unit_get_item);
      tolua_function(tolua_S, "add_item", tolua_unit_add_item);
      tolua_variable(tolua_S, "items", tolua_unit_get_items, 0);
      tolua_function(tolua_S, "get_pooled", tolua_unit_get_pooled);
      tolua_function(tolua_S, "use_pooled", tolua_unit_use_pooled);

      // skills:
      tolua_function(tolua_S, "get_skill", tolua_unit_getskill);
      tolua_function(tolua_S, "eff_skill", tolua_unit_effskill);
      tolua_function(tolua_S, "set_skill", tolua_unit_setskill);

      tolua_function(tolua_S, "add_notice", tolua_unit_addnotice);

      // npc logic:
      tolua_function(tolua_S, "add_handler", tolua_unit_addhandler);

      tolua_function(tolua_S, "set_racename", tolua_unit_setracename);
      tolua_function(tolua_S, "add_spell", tolua_unit_addspell);
      tolua_function(tolua_S, "remove_spell", tolua_unit_removespell);
      tolua_function(tolua_S, "cast_spell", tolua_unit_castspell);

      tolua_variable(tolua_S, "magic", tolua_unit_get_magic, tolua_unit_set_magic);
      tolua_variable(tolua_S, "aura", tolua_unit_get_aura, tolua_unit_set_aura);
      tolua_variable(tolua_S, "building", tolua_unit_get_building, tolua_unit_set_building);
      tolua_variable(tolua_S, "ship", tolua_unit_get_ship, tolua_unit_set_ship);
      tolua_variable(tolua_S, "region", tolua_unit_get_region, tolua_unit_set_region);
      tolua_variable(tolua_S, "spells", tolua_unit_get_spells, 0);
      tolua_variable(tolua_S, "number", tolua_unit_get_number, tolua_unit_set_number);
      tolua_variable(tolua_S, "race", tolua_unit_get_race, tolua_unit_set_race);
      tolua_variable(tolua_S, "hp_max", tolua_unit_get_hpmax, 0);

      tolua_variable(tolua_S, "objects", tolua_unit_get_objects, 0);
    }
    tolua_endmodule(tolua_S);
  }
  tolua_endmodule(tolua_S);
}
