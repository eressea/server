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

#include "bindings.h"
#include <config.h>

#include <kernel/eressea.h>

#include <util/attrib.h>
#include <util/log.h>
#include <kernel/alliance.h>
#include <kernel/unit.h>
#include <kernel/terrain.h>
#include <kernel/message.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/building.h>
#include <kernel/race.h>
#include <kernel/ship.h>
#include <kernel/teleport.h>
#include <kernel/faction.h>
#include <kernel/save.h>

#include <gamecode/summary.h>
#include <gamecode/laws.h>
#include <gamecode/monster.h>

#include <spells/spells.h>

#include <tolua.h>
#include <lua.h>

#include <time.h>

static int tolua_get_unit_name(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, self->name);
  return 1;
}

static int tolua_set_unit_name(lua_State* tolua_S)
{
  unit* self = (unit*)tolua_tousertype(tolua_S, 1, 0);
  unit_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_building_name(lua_State* tolua_S)
{
  building* self = (building*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, building_getname(self));
  return 1;
}

static int tolua_set_building_name(lua_State* tolua_S)
{
  building* self = (building*)tolua_tousertype(tolua_S, 1, 0);
  building_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_region_name(lua_State* tolua_S)
{
  region* self = (region*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, region_getname(self));
  return 1;
}

static int tolua_set_region_name(lua_State* tolua_S)
{
  region* self = (region*)tolua_tousertype(tolua_S, 1, 0);
  region_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_ship_name(lua_State* tolua_S)
{
  ship* self = (ship*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, ship_getname(self));
  return 1;
}

static int tolua_set_ship_name(lua_State* tolua_S)
{
  ship* self = (ship*)tolua_tousertype(tolua_S, 1, 0);
  ship_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_unit_faction(lua_State* tolua_S)
{
  unit* self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, (void*)self->faction, "faction");
  return 1;
}

static int tolua_set_unit_faction(lua_State* tolua_S)
{
  unit * self = (unit*) tolua_tousertype(tolua_S, 1, 0);
  faction * f = (faction *)tolua_tousertype(tolua_S, 2, 0);
  u_setfaction(self, f);
  return 0;
}

static int tolua_unitlist_next(lua_State *tolua_S)
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

static int tolua_factionlist_next(lua_State *tolua_S)
{
  faction** faction_ptr = (faction **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  faction * f = *faction_ptr;
  if (f != NULL) {
    tolua_pushusertype(tolua_S, (void*)f, "faction");
    *faction_ptr = f->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int tolua_factionlist_iter(lua_State *tolua_S)
{
  faction_list** faction_ptr = (faction_list **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  faction_list* flist = *faction_ptr;
  if (flist != NULL) {
    tolua_pushusertype(tolua_S, (void*)flist->data, "faction");
    *faction_ptr = flist->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int tolua_regionlist_next(lua_State *tolua_S)
{
  region** region_ptr = (region **)lua_touserdata(tolua_S, lua_upvalueindex(1));
  region * u = *region_ptr;
  if (u != NULL) {
    tolua_pushusertype(tolua_S, (void*)u, "region");
    *region_ptr = u->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int tolua_unitlist_nextf(lua_State *tolua_S)
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

static int tolua_get_faction_units(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  unit ** unit_ptr = (unit**)lua_newuserdata(tolua_S, sizeof(unit *));

  luaL_getmetatable(tolua_S, "unit");
  lua_setmetatable(tolua_S, -2);

  *unit_ptr = self->units;

  lua_pushcclosure(tolua_S, tolua_unitlist_nextf, 1);
  return 1;
}

static int tolua_get_region_units(lua_State* tolua_S)
{
  region * self = (region *)tolua_tousertype(tolua_S, 1, 0);
  unit ** unit_ptr = (unit**)lua_newuserdata(tolua_S, sizeof(unit *));

  luaL_getmetatable(tolua_S, "unit");
  lua_setmetatable(tolua_S, -2);

  *unit_ptr = self->units;

  lua_pushcclosure(tolua_S, tolua_unitlist_next, 1);
  return 1;
}

static int
tolua_read_orders(lua_State* tolua_S)
{
  const char * filename = tolua_tostring(tolua_S, 1, 0);
  int result;
  ++turn;
  result = readorders(filename);
  lua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_message_unit(lua_State* tolua_S)
{
  unit * sender = (unit *)tolua_tousertype(tolua_S, 1, 0);
  unit * target = (unit *)tolua_tousertype(tolua_S, 2, 0);
  const char * str = tolua_tostring(tolua_S, 3, 0);

  deliverMail(target->faction, sender->region, sender, str, target);
  return 0;
}

static int
tolua_message_faction(lua_State * tolua_S)
{
  unit * sender = (unit *)tolua_tousertype(tolua_S, 1, 0);
  faction * target = (faction *)tolua_tousertype(tolua_S, 2, 0);
  const char * str = tolua_tostring(tolua_S, 3, 0);

  deliverMail(target, sender->region, sender, str, NULL);
  return 0;
}

static int
tolua_message_region(lua_State * tolua_S)
{
  unit * sender = (unit *)tolua_tousertype(tolua_S, 1, 0);
  const char * str = tolua_tostring(tolua_S, 2, 0);

  ADDMSG(&sender->region->msgs, msg_message("mail_result", "unit message", sender, str));

  return 0;
}

static void
free_script(attrib * a)
{
  lua_State * L = (lua_State *)global.vm_state;
  if (a->data.i>0) {
    luaL_unref(L, LUA_REGISTRYINDEX, a->data.i);
  }
}

attrib_type at_script = {
  "script",
  NULL, free_script, NULL,
  NULL, NULL, ATF_UNIQUE
};

static int
call_script(lua_State * L, struct unit * u)
{
  const attrib * a = a_findc(u->attribs, &at_script);
  if (a==NULL) a = a_findc(u->race->attribs, &at_script);
  if (a!=NULL && a->data.i>0) {
    if (lua_pcall(L, 1, 0, 0)!=0) {
      const char* error = lua_tostring(L, -1);
      log_error(("call_script (%s): %s", unitname(u), error));
      lua_pop(L, 1);
    }
  }
  return -1;
}

static void
setscript(lua_State * tolua_S, struct attrib ** ap)
{
  attrib * a = a_find(*ap, &at_script);
  if (a == NULL) {
    a = a_add(ap, a_new(&at_script));
  } else if (a->data.i>0) {
    luaL_unref(tolua_S, LUA_REGISTRYINDEX, a->data.i);
  }
  a->data.i = luaL_ref(tolua_S, LUA_REGISTRYINDEX);
}

static int
tolua_update_guards(lua_State * tolua_S)
{
  update_guards();
  return 0;
}

static int
tolua_levitate_ship(lua_State * tolua_S)
{
  ship * sh = (ship *)tolua_tousertype(tolua_S, 1, 0);
  unit * mage = (unit *)tolua_tousertype(tolua_S, 2, 0);
  double power = (double)tolua_tonumber(tolua_S, 3, 0);
  int duration = (int)tolua_tonumber(tolua_S, 4, 0);
  int cno = levitate_ship(sh, mage, power, duration);
  tolua_pushnumber(tolua_S, (lua_Number)cno);
  return 1;
}

static int
tolua_set_unitscript(lua_State * tolua_S)
{
  struct unit * u = (struct unit *)tolua_tousertype(tolua_S, 1, 0);
  if (u) {
    lua_pushvalue(tolua_S, 2);
    setscript(tolua_S, &u->attribs);
    lua_pop(tolua_S, 1);
  }
  return 0;
}

static int
tolua_set_racescript(lua_State * tolua_S)
{
  const char * rcname = tolua_tostring(tolua_S, 1, 0);
  race * rc = rc_find(rcname);

  if (rc!=NULL) {
    lua_pushvalue(tolua_S, 2);
    setscript(tolua_S, &rc->attribs);
    lua_pop(tolua_S, 1);
  }
  return 0;
}

static int
tolua_spawn_dragons(lua_State * tolua_S)
{
  spawn_dragons();
  return 0;
}

static int
tolua_spawn_undead(lua_State * tolua_S)
{
  spawn_undead();
  return 0;
}

static int
tolua_spawn_braineaters(lua_State * tolua_S)
{
  float chance = (float)tolua_tonumber(tolua_S, 1, 0);
  spawn_braineaters(chance);
  return 0;
}

static int
tolua_planmonsters(lua_State * tolua_S)
{
  faction * f = get_monsters();

  if (f!=NULL) {
    unit * u;
    plan_monsters();
    for (u=f->units;u;u=u->nextF) {
      call_script(tolua_S, u);
    }
  }

  return 0;
}

static int
tolua_init_reports(lua_State* tolua_S)
{
  int result = init_reports();
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_write_report(lua_State* tolua_S)
{
  faction * f = (faction * )tolua_tousertype(tolua_S, 1, 0);
  time_t ltime = time(0);
  int result = write_reports(f, ltime);

  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_write_reports(lua_State* tolua_S)
{
  int result;

  init_reports();
  result = reports();
  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_process_orders(lua_State* tolua_S)
{
  ++turn;
  processorders();
  return 0;
}
static int
tolua_write_passwords(lua_State* tolua_S)
{
  int result = writepasswd();
  lua_pushnumber(tolua_S, (lua_Number)result);
  return 0;
}

static struct summary * sum_begin = 0;

static int
tolua_init_summary(lua_State* tolua_S)
{
  sum_begin = make_summary();
  return 0;
}

static int
tolua_write_summary(lua_State* tolua_S)
{
  if (sum_begin) {
    struct summary * sum_end = make_summary();
    report_summary(sum_end, sum_begin, false);
    report_summary(sum_end, sum_begin, true);
    return 0;
  }
  return 0;
}

static int
tolua_free_game(lua_State* tolua_S)
{
  free_gamedata();
  return 0;
}

static int
tolua_write_game(lua_State* tolua_S)
{
  const char * filename = tolua_tostring(tolua_S, 1, 0);
  const char * mode =  tolua_tostring(tolua_S, 2, 0);

  int result, m = IO_TEXT;
  if (strcmp(mode, "text")!=0) m = IO_BINARY;
  remove_empty_factions(true);
  result = writegame(filename, m);

  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}

static int
tolua_read_game(lua_State* tolua_S)
{
  const char * filename = tolua_tostring(tolua_S, 1, 0);
  const char * mode =  tolua_tostring(tolua_S, 2, 0);

  int rv, m = IO_TEXT;
  if (strcmp(mode, "text")!=0) m = IO_BINARY;
  rv = readgame(filename, m, false);

  tolua_pushnumber(tolua_S, (lua_Number)rv);
  return 1;
}

static int
tolua_get_faction(lua_State* tolua_S)
{
  int no = (int)tolua_tonumber(tolua_S, 1, 0);
  faction * f = findfaction(no);

  tolua_pushusertype(tolua_S, f, "faction");
  return 1;
}

static int
tolua_get_alliance(lua_State* tolua_S)
{
  int no = (int)tolua_tonumber(tolua_S, 1, 0);
  alliance * f = findalliance(no);

  tolua_pushusertype(tolua_S, f, "alliance");
  return 1;
}

static int
tolua_get_unit(lua_State* tolua_S)
{
  int no = (int)tolua_tonumber(tolua_S, 1, 0);
  unit * u = findunit(no);

  tolua_pushusertype(tolua_S, u, "unit");
  return 1;
}

static int
tolua_unit_create(lua_State* tolua_S)
{
  faction * f = (faction *)tolua_tousertype(tolua_S, 1, 0);
  region * r = (region *)tolua_tousertype(tolua_S, 2, 0);
  unit * u = createunit(r, f, 0, f->race);

  tolua_pushusertype(tolua_S, u, "unit");
  return 1;
}

static int
tolua_get_faction_maxheroes(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)maxheroes(self));
  return 1;
}

static int
tolua_get_faction_heroes(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)countheroes(self));
  return 1;
}

static int
tolua_get_faction_score(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->score);
  return 1;
}

static int
tolua_get_faction_id(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->no);
  return 1;
}

static int
tolua_get_faction_age(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->age);
  return 1;
}

static int
tolua_get_faction_flags(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->flags);
  return 1;
}

static int
tolua_get_faction_options(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->options);
  return 1;
}

static int
tolua_get_faction_lastturn(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->lastorders);
  return 1;
}


static int
tolua_faction_renumber(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  int no = (int)tolua_tonumber(tolua_S, 2, 0);

  renumber_faction(self, no);
  return 0;
}

static int
tolua_faction_get_policy(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  faction * other = (faction *)tolua_tousertype(tolua_S, 2, 0);
  const char * policy = tolua_tostring(tolua_S, 3, 0);

  int result = 0, mode;
  for (mode=0;helpmodes[mode].name!=NULL;++mode) {
    if (strcmp(policy, helpmodes[mode].name)==0) {
      result = get_alliance(self, other) & mode;
      break;
    }
  }

  tolua_pushnumber(tolua_S, (lua_Number)result);
  return 1;
}


static int
tolua_faction_set_policy(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);
  faction * other = (faction *)tolua_tousertype(tolua_S, 2, 0);
  const char * policy = tolua_tostring(tolua_S, 3, 0);
  int value = tolua_toboolean(tolua_S, 4, 0);

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
tolua_faction_get_origin(lua_State* tolua_S)
{
  faction * self = (faction *)tolua_tousertype(tolua_S, 1, 0);

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

  tolua_pushnumber(tolua_S, (lua_Number)x);
  tolua_pushnumber(tolua_S, (lua_Number)y);
  return 2;
}

static int
tolua_region_create(lua_State* tolua_S)
{
  short x = (short)tolua_tonumber(tolua_S, 1, 0);
  short y = (short)tolua_tonumber(tolua_S, 2, 0);
  const char * tname = tolua_tostring(tolua_S, 3, 0);

  const terrain_type * terrain = get_terrain(tname);
  region * r = findregion(x, y);
  region * result = r;

  if (terrain==NULL) {
    if (r!=NULL) {
      if (r->units!=NULL) {
        /* TODO: error message */
        result = NULL;
      }
    }
  }
  if (r==NULL) {
    result = new_region(x, y, 0);
  }
  if (result) {
    terraform_region(result, terrain);
  }

  tolua_pushusertype(tolua_S, result, "region");
  return 1;
}

static int
tolua_alliance_create(lua_State* tolua_S)
{
  int id = (int)tolua_tonumber(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);

  alliance * alli = makealliance(id, name);

  tolua_pushusertype(tolua_S, alli, "alliance");
  return 1;
}

static int
tolua_faction_create(lua_State* tolua_S)
{
  const char * email = tolua_tostring(tolua_S, 1, 0);
  const char * racename = tolua_tostring(tolua_S, 2, 0);
  const char * lang = tolua_tostring(tolua_S, 3, 0);
  struct locale * loc = find_locale(lang);
  faction * f = NULL;
  const struct race * frace = findrace(racename, default_locale);
  if (frace==NULL) frace = findrace(racename, find_locale("de"));
  if (frace==NULL) frace = findrace(racename, find_locale("en"));
  if (frace!=NULL) {
    f = addfaction(email, NULL, frace, loc, 0);
  }
  tolua_pushusertype(tolua_S, f, "faction");
  return 1;
}

static int
tolua_get_regions(lua_State* tolua_S)
{
  region ** region_ptr = (region**)lua_newuserdata(tolua_S, sizeof(region *));

  luaL_getmetatable(tolua_S, "region");
  lua_setmetatable(tolua_S, -2);

  *region_ptr = regions;

  lua_pushcclosure(tolua_S, tolua_regionlist_next, 1);
  return 1;
}

static int
tolua_get_factions(lua_State* tolua_S)
{
  faction ** faction_ptr = (faction**)lua_newuserdata(tolua_S, sizeof(faction *));

  luaL_getmetatable(tolua_S, "faction");
  lua_setmetatable(tolua_S, -2);

  *faction_ptr = factions;

  lua_pushcclosure(tolua_S, tolua_factionlist_next, 1);
  return 1;
}

static int
tolua_get_alliance_factions(lua_State* tolua_S)
{
  alliance * self = (alliance *)tolua_tousertype(tolua_S, 1, 0);
  faction_list ** faction_ptr = (faction_list**)lua_newuserdata(tolua_S, sizeof(faction_list *));

  luaL_getmetatable(tolua_S, "faction_list");
  lua_setmetatable(tolua_S, -2);

  *faction_ptr = self->members;

  lua_pushcclosure(tolua_S, tolua_factionlist_iter, 1);
  return 1;
}

static int tolua_get_faction_password(lua_State* tolua_S)
{
  faction* self = (faction*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, faction_getpassword(self));
  return 1;
}

static int tolua_set_faction_password(lua_State* tolua_S)
{
  faction* self = (faction*)tolua_tousertype(tolua_S, 1, 0);
  faction_setpassword(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_faction_email(lua_State* tolua_S)
{
  faction* self = (faction*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, faction_getemail(self));
  return 1;
}

static int tolua_set_faction_email(lua_State* tolua_S)
{
  faction* self = (faction*)tolua_tousertype(tolua_S, 1, 0);
  faction_setemail(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_faction_locale(lua_State* tolua_S)
{
  faction* self = (faction*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, locale_name(self->locale));
  return 1;
}

static int tolua_set_faction_locale(lua_State* tolua_S)
{
  faction* self = (faction*)tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  self->locale = find_locale(name);
  return 0;
}

static int tolua_get_faction_race(lua_State* tolua_S)
{
  faction* self = (faction*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, self->race->_name[0]);
  return 1;
}

static int tolua_set_faction_race(lua_State* tolua_S)
{
  faction* self = (faction*)tolua_tousertype(tolua_S, 1, 0);
  const char * name = tolua_tostring(tolua_S, 2, 0);
  race * rc = rc_find(name);
  if (rc!=NULL) {
    self->race = rc;
  }

  return 0;
}

static int tolua_get_faction_alliance(lua_State* tolua_S)
{
  faction* self = (faction*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushusertype(tolua_S, self->alliance, "alliance");
  return 1;
}

static int tolua_set_faction_alliance(lua_State* tolua_S)
{
  faction* self = (faction*)tolua_tousertype(tolua_S, 1, 0);
  alliance* alli = (alliance*)tolua_tousertype(tolua_S, 2, 0);

  if (!self->alliance) {
    setalliance(self, alli);
  }

  return 0;
}

static int tolua_get_alliance_id(lua_State* tolua_S)
{
  alliance* self = (alliance*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushnumber(tolua_S, (lua_Number)self->id);
  return 1;
}

static int tolua_get_alliance_name(lua_State* tolua_S)
{
  alliance* self = (alliance*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, self->name);
  return 1;
}

static int tolua_set_alliance_name(lua_State* tolua_S)
{
  alliance* self = (alliance*)tolua_tousertype(tolua_S, 1, 0);
  alliance_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_faction_name(lua_State* tolua_S)
{
  faction* self = (faction*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, faction_getname(self));
  return 1;
}

static int tolua_set_faction_name(lua_State* tolua_S)
{
  faction* self = (faction*)tolua_tousertype(tolua_S, 1, 0);
  faction_setname(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}

static int tolua_get_faction_info(lua_State* tolua_S)
{
  faction* self = (faction*) tolua_tousertype(tolua_S, 1, 0);
  tolua_pushstring(tolua_S, faction_getbanner(self));
  return 1;
}

static int tolua_set_faction_info(lua_State* tolua_S)
{
  faction* self = (faction*)tolua_tousertype(tolua_S, 1, 0);
  faction_setbanner(self, tolua_tostring(tolua_S, 2, 0));
  return 0;
}


int tolua_eressea_open(lua_State* tolua_S)
{
  tolua_open(tolua_S);

  /* register user types */
  tolua_usertype(tolua_S, "unit");
  tolua_usertype(tolua_S, "alliance");
  tolua_usertype(tolua_S, "faction");
  tolua_usertype(tolua_S, "region");
  tolua_usertype(tolua_S, "ship");
  tolua_usertype(tolua_S, "building");
  tolua_usertype(tolua_S, "faction_list");

  tolua_module(tolua_S, NULL, 0);
  tolua_beginmodule(tolua_S, NULL);
  {
    tolua_cclass(tolua_S, "faction", "faction", "", NULL);
    tolua_beginmodule(tolua_S, "faction");
    {
      tolua_variable(tolua_S, "name", tolua_get_faction_name, tolua_set_faction_name);
      tolua_variable(tolua_S, "units", tolua_get_faction_units, NULL);
      tolua_variable(tolua_S, "heroes", tolua_get_faction_heroes, NULL);
      tolua_variable(tolua_S, "maxheroes", tolua_get_faction_maxheroes, NULL);
      tolua_variable(tolua_S, "password", tolua_get_faction_password, tolua_set_faction_password);
      tolua_variable(tolua_S, "email", tolua_get_faction_email, tolua_set_faction_email);
      tolua_variable(tolua_S, "locale", tolua_get_faction_locale, tolua_set_faction_locale);
      tolua_variable(tolua_S, "race", tolua_get_faction_race, tolua_set_faction_race);
      tolua_variable(tolua_S, "alliance", tolua_get_faction_alliance, tolua_set_faction_alliance);
      tolua_variable(tolua_S, "score", tolua_get_faction_score, NULL);
      tolua_variable(tolua_S, "id", tolua_get_faction_id, NULL);
      tolua_variable(tolua_S, "age", tolua_get_faction_age, NULL);
      tolua_variable(tolua_S, "options", tolua_get_faction_options, NULL);
      tolua_variable(tolua_S, "flags", tolua_get_faction_flags, NULL);
      tolua_variable(tolua_S, "lastturn", tolua_get_faction_lastturn, NULL);
 
      tolua_function(tolua_S, "set_policy", tolua_faction_set_policy);
      tolua_function(tolua_S, "get_policy", tolua_faction_get_policy);
      tolua_function(tolua_S, "get_origin", tolua_faction_get_origin);

      tolua_function(tolua_S, "renumber", tolua_faction_renumber);
      tolua_function(tolua_S, "create", tolua_faction_create);
    }
    tolua_endmodule(tolua_S);

    tolua_cclass(tolua_S, "alliance", "alliance", "", NULL);
    tolua_beginmodule(tolua_S, "alliance");
    {
      tolua_variable(tolua_S, "name", tolua_get_alliance_name, tolua_set_alliance_name);
      tolua_variable(tolua_S, "id", tolua_get_alliance_id, NULL);
      tolua_variable(tolua_S, "factions", tolua_get_alliance_factions, NULL);
      tolua_function(tolua_S, "create", tolua_alliance_create);
    }
    tolua_endmodule(tolua_S);

    tolua_cclass(tolua_S, "region", "region", "", NULL);
    tolua_beginmodule(tolua_S, "region");
    {
      tolua_variable(tolua_S, "name", tolua_get_region_name, tolua_set_region_name);
      tolua_variable(tolua_S, "units", tolua_get_region_units, NULL);
      tolua_function(tolua_S, "create", tolua_region_create);
    }
    tolua_endmodule(tolua_S);

    tolua_cclass(tolua_S, "building", "building", "", NULL);
    tolua_beginmodule(tolua_S, "building");
    {
      tolua_variable(tolua_S, "name", tolua_get_building_name, tolua_set_building_name);
    }
    tolua_endmodule(tolua_S);

    tolua_cclass(tolua_S, "ship", "ship", "", NULL);
    tolua_beginmodule(tolua_S, "ship");
    {
      tolua_variable(tolua_S, "name", tolua_get_ship_name, tolua_set_ship_name);
    }
    tolua_endmodule(tolua_S);

    tolua_cclass(tolua_S, "unit", "unit", "", NULL);
    tolua_beginmodule(tolua_S, "unit");
    {
      tolua_variable(tolua_S, "name", tolua_get_unit_name, tolua_set_unit_name);
      tolua_variable(tolua_S, "faction", tolua_get_unit_faction, tolua_set_unit_faction);
      tolua_function(tolua_S, "create", tolua_unit_create);
    }
    tolua_endmodule(tolua_S);

    // deprecated_function(tolua_S, "add_faction");
    // deprecated_function(tolua_S, "faction_origin");
    tolua_function(tolua_S, "factions", tolua_get_factions);
    tolua_function(tolua_S, "regions", tolua_get_regions);

    tolua_function(tolua_S, "read_game", tolua_read_game);
    tolua_function(tolua_S, "write_game", tolua_write_game);
    tolua_function(tolua_S, "free_game", tolua_free_game);

    tolua_function(tolua_S, "read_orders", tolua_read_orders);
    tolua_function(tolua_S, "process_orders", tolua_process_orders);

    tolua_function(tolua_S, "init_reports", tolua_init_reports);
    tolua_function(tolua_S, "write_reports", tolua_write_reports);
    tolua_function(tolua_S, "write_report", tolua_write_report);

    tolua_function(tolua_S, "init_summary", tolua_init_summary);
    tolua_function(tolua_S, "write_summary", tolua_write_summary);
    tolua_function(tolua_S, "write_passwords", tolua_write_passwords),

    tolua_function(tolua_S, "message_unit", tolua_message_unit);
    tolua_function(tolua_S, "message_faction", tolua_message_faction);
    tolua_function(tolua_S, "message_region", tolua_message_region);

    tolua_function(tolua_S, "get_faction", tolua_get_faction);
    tolua_function(tolua_S, "get_unit", tolua_get_unit);
    tolua_function(tolua_S, "get_alliance", tolua_get_alliance);

    /* scripted monsters */
    tolua_function(tolua_S, "plan_monsters", tolua_planmonsters);
    tolua_function(tolua_S, "spawn_braineaters", tolua_spawn_braineaters);
    tolua_function(tolua_S, "spawn_undead", tolua_spawn_undead);
    tolua_function(tolua_S, "spawn_dragons", tolua_spawn_dragons);

    tolua_function(tolua_S, "set_race_brain", tolua_set_racescript);
    tolua_function(tolua_S, "set_unit_brain", tolua_set_unitscript);

    /* spells and stuff */
    tolua_function(tolua_S, "levitate_ship", tolua_levitate_ship);

    tolua_function(tolua_S, "update_guards", tolua_update_guards);
  }
  tolua_endmodule(tolua_S);
  return 1;
}
