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
#include "bind_faction.h"
#include "bind_unit.h"
#include "bindings.h"

#include <kernel/alliance.h>
#include <kernel/eressea.h>
#include <kernel/unit.h>
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/race.h>

#include <util/language.h>

#include <lua.h>
#include <tolua.h>


int tolua_factionlist_next(lua_State *L)
{
  faction** faction_ptr = (faction **)lua_touserdata(L, lua_upvalueindex(1));
  faction * f = *faction_ptr;
  if (f != NULL) {
    tolua_pushusertype(L, (void*)f, "faction");
    *faction_ptr = f->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

int tolua_factionlist_iter(lua_State *L)
{
  faction_list** faction_ptr = (faction_list **)lua_touserdata(L, lua_upvalueindex(1));
  faction_list* flist = *faction_ptr;
  if (flist != NULL) {
    tolua_pushusertype(L, (void*)flist->data, "faction");
    *faction_ptr = flist->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int tolua_faction_get_units(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  unit ** unit_ptr = (unit**)lua_newuserdata(L, sizeof(unit *));

  luaL_getmetatable(L, "unit");
  lua_setmetatable(L, -2);

  *unit_ptr = self->units;

  lua_pushcclosure(L, tolua_unitlist_nextf, 1);
  return 1;
}

int tolua_faction_add_item(lua_State *L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  const char * iname = tolua_tostring(L, 2, 0);
  int number = (int)tolua_tonumber(L, 3, 0);
  int result = -1;

  if (iname!=NULL) {
    const item_type * itype = it_find(iname);
    if (itype!=NULL) {
      item * i = i_change(&self->items, itype, number);
      result = i?i->number:0;
    } // if (itype!=NULL)
  }
  lua_pushnumber(L, result);
  return 1;
}

static int
tolua_faction_get_maxheroes(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)maxheroes(self));
  return 1;
}

static int
tolua_faction_get_heroes(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)countheroes(self));
  return 1;
}

static int
tolua_faction_get_score(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->score);
  return 1;
}

static int
tolua_faction_get_id(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->no);
  return 1;
}

static int
tolua_faction_set_id(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  int id = (int)tolua_tonumber(L, 2, 0);
  if (findfaction(id)==NULL) {
    renumber_faction(self, id);
    lua_pushboolean(L, 1);
  } else {
    lua_pushboolean(L, 0);
  }
  return 1;
}

static int
tolua_faction_get_age(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->age);
  return 1;
}

static int
tolua_faction_set_age(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  int age = (int)tolua_tonumber(L, 2, 0);
  self->age = age;
  return 0;
}

static int
tolua_faction_get_flags(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->flags);
  return 1;
}

static int
tolua_faction_get_options(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->options);
  return 1;
}

static int
tolua_faction_set_options(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  int options = (int)tolua_tonumber(L, 2, self->options);
  self->options = options;
  return 1;
}

static int
tolua_faction_get_lastturn(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->lastorders);
  return 1;
}

static int
tolua_faction_renumber(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  int no = (int)tolua_tonumber(L, 2, 0);

  renumber_faction(self, no);
  return 0;
}

static int
tolua_faction_get_objects(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  tolua_pushusertype(L, (void*)&self->attribs, "hashtable");
  return 1;
}

static int
tolua_faction_get_policy(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  faction * other = (faction *)tolua_tousertype(L, 2, 0);
  const char * policy = tolua_tostring(L, 3, 0);

  int result = 0, mode;
  for (mode=0;helpmodes[mode].name!=NULL;++mode) {
    if (strcmp(policy, helpmodes[mode].name)==0) {
      result = get_alliance(self, other) & mode;
      break;
    }
  }

  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}


static int
tolua_faction_set_policy(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  faction * other = (faction *)tolua_tousertype(L, 2, 0);
  const char * policy = tolua_tostring(L, 3, 0);
  int value = tolua_toboolean(L, 4, 0);

  int mode;
  for (mode=0;helpmodes[mode].name!=NULL;++mode) {
    if (strcmp(policy, helpmodes[mode].name)==0) {
      if (value) {
        set_alliance(self, other, get_alliance(self, other) | helpmodes[mode].status);
      } else {
        set_alliance(self, other, get_alliance(self, other) & ~helpmodes[mode].status);
      }
      break;
    }
  }

  return 0;
}

static int
tolua_faction_get_origin(lua_State* L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);

  ursprung * origin = self->ursprung;
  int x, y;
  while (origin!=NULL && origin->id!=0) {
    origin = origin->next;
  }
  if (origin) {
    x = origin->x;
    y = origin->y;
  } else {
    x = 0;
    y = 0;
  }

  tolua_pushnumber(L, (lua_Number)x);
  tolua_pushnumber(L, (lua_Number)y);
  return 2;
}

static int
tolua_faction_create(lua_State* L)
{
  const char * email = tolua_tostring(L, 1, 0);
  const char * racename = tolua_tostring(L, 2, 0);
  const char * lang = tolua_tostring(L, 3, 0);
  struct locale * loc = find_locale(lang);
  faction * f = NULL;
  const struct race * frace = findrace(racename, default_locale);
  if (frace==NULL) frace = findrace(racename, find_locale("de"));
  if (frace==NULL) frace = findrace(racename, find_locale("en"));
  if (frace!=NULL) {
    f = addfaction(email, NULL, frace, loc, 0);
  }
  tolua_pushusertype(L, f, "faction");
  return 1;
}

static int tolua_faction_get_password(lua_State* L)
{
  faction* self = (faction*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, faction_getpassword(self));
  return 1;
}

static int tolua_faction_set_password(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  faction_setpassword(self, tolua_tostring(L, 2, 0));
  return 0;
}

static int tolua_faction_get_email(lua_State* L)
{
  faction* self = (faction*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, faction_getemail(self));
  return 1;
}

static int tolua_faction_set_email(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  faction_setemail(self, tolua_tostring(L, 2, 0));
  return 0;
}

static int tolua_faction_get_locale(lua_State* L)
{
  faction* self = (faction*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, locale_name(self->locale));
  return 1;
}

static int tolua_faction_set_locale(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  const char * name = tolua_tostring(L, 2, 0);
  self->locale = find_locale(name);
  return 0;
}

static int tolua_faction_get_race(lua_State* L)
{
  faction* self = (faction*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, self->race->_name[0]);
  return 1;
}

static int tolua_faction_set_race(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  const char * name = tolua_tostring(L, 2, 0);
  race * rc = rc_find(name);
  if (rc!=NULL) {
    self->race = rc;
  }

  return 0;
}

static int tolua_faction_get_name(lua_State* L)
{
  faction* self = (faction*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, faction_getname(self));
  return 1;
}

static int tolua_faction_set_name(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  faction_setname(self, tolua_tostring(L, 2, 0));
  return 0;
}

static int tolua_faction_get_info(lua_State* L)
{
  faction* self = (faction*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, faction_getbanner(self));
  return 1;
}

static int tolua_faction_set_info(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  faction_setbanner(self, tolua_tostring(L, 2, 0));
  return 0;
}

static int tolua_faction_get_alliance(lua_State* L)
{
  faction* self = (faction*) tolua_tousertype(L, 1, 0);
  tolua_pushusertype(L, self->alliance, "alliance");
  return 1;
}

static int tolua_faction_set_alliance(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  alliance* alli = (alliance*)tolua_tousertype(L, 2, 0);

  if (!self->alliance) {
    setalliance(self, alli);
  }

  return 0;
}

static int tolua_faction_get_items(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  item ** item_ptr = (item **)lua_newuserdata(L, sizeof(item *));

  luaL_getmetatable(L, "item");
  lua_setmetatable(L, -2);

  *item_ptr = self->items;

  lua_pushcclosure(L, tolua_itemlist_next, 1);

  return 1;
}

static int
tolua_faction_tostring(lua_State *L)
{
  faction * self = (faction *)tolua_tousertype(L, 1, 0);
  lua_pushstring(L, factionname(self));
  return 1;
}

static int tolua_faction_get_spells(lua_State* L)
{
  faction* self = (faction*)tolua_tousertype(L, 1, 0);
  spell_list * slist = self->spellbook;
  if (slist) {
    spell_list ** spell_ptr = (spell_list **)lua_newuserdata(L, sizeof(spell_list *));
    luaL_getmetatable(L, "spell_list");
    lua_setmetatable(L, -2);

    *spell_ptr = slist;
    lua_pushcclosure(L, tolua_spelllist_next, 1);
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

void
tolua_faction_open(lua_State* L)
{
  /* register user types */
  tolua_usertype(L, "faction");
  tolua_usertype(L, "faction_list");

  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_cclass(L, "faction", "faction", "", NULL);
    tolua_beginmodule(L, "faction");
    {
      tolua_function(L, "__tostring", tolua_faction_tostring);

      tolua_variable(L, "name", tolua_faction_get_name, tolua_faction_set_name);
      tolua_variable(L, "info", tolua_faction_get_info, tolua_faction_set_info);
      tolua_variable(L, "units", tolua_faction_get_units, NULL);
      tolua_variable(L, "heroes", tolua_faction_get_heroes, NULL);
      tolua_variable(L, "spells", tolua_faction_get_spells, 0);
      tolua_variable(L, "maxheroes", tolua_faction_get_maxheroes, NULL);
      tolua_variable(L, "password", tolua_faction_get_password, tolua_faction_set_password);
      tolua_variable(L, "email", tolua_faction_get_email, tolua_faction_set_email);
      tolua_variable(L, "locale", tolua_faction_get_locale, tolua_faction_set_locale);
      tolua_variable(L, "race", tolua_faction_get_race, tolua_faction_set_race);
      tolua_variable(L, "alliance", tolua_faction_get_alliance, tolua_faction_set_alliance);
      tolua_variable(L, "score", tolua_faction_get_score, NULL);
      tolua_variable(L, "id", tolua_faction_get_id, tolua_faction_set_id);
      tolua_variable(L, "age", tolua_faction_get_age, tolua_faction_set_age);
      tolua_variable(L, "options", tolua_faction_get_options, tolua_faction_set_options);
      tolua_variable(L, "flags", tolua_faction_get_flags, NULL);
      tolua_variable(L, "lastturn", tolua_faction_get_lastturn, NULL);
 
      tolua_function(L, "set_policy", tolua_faction_set_policy);
      tolua_function(L, "get_policy", tolua_faction_get_policy);
      tolua_function(L, "get_origin", tolua_faction_get_origin);

      tolua_function(L, "add_item", tolua_faction_add_item);
      tolua_variable(L, "items", tolua_faction_get_items, NULL);

      tolua_function(L, "renumber", tolua_faction_renumber);
      tolua_function(L, "create", tolua_faction_create);
#ifdef TODO
      def("faction_origin", &faction_getorigin, pure_out_value(_2) + pure_out_value(_3)),

      .def_readwrite("subscription", &faction::subscription)

      .property("x", &faction_getorigin_x, &faction_setorigin_x)
      .property("y", &faction_getorigin_y, &faction_setorigin_y)

      .def("add_notice", &faction_addnotice)
#endif
      tolua_variable(L, "objects", tolua_faction_get_objects, NULL);
    }
    tolua_endmodule(L);
  }
  tolua_endmodule(L);
}
