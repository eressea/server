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

#include <platform.h>
#include "bindings.h"
#include "bind_unit.h"
#include "bind_faction.h"
#include "bind_region.h"
#include "helpers.h"

#include <kernel/config.h>

#include <kernel/alliance.h>
#include <kernel/skill.h>
#include <kernel/equipment.h>
#include <kernel/calendar.h>
#include <kernel/unit.h>
#include <kernel/terrain.h>
#include <kernel/message.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/building.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/ship.h>
#include <kernel/teleport.h>
#include <kernel/faction.h>
#include <kernel/save.h>

#include <gamecode/creport.h>
#include <gamecode/economy.h>
#include <gamecode/summary.h>
#include <gamecode/laws.h>
#include <gamecode/monster.h>
#include <gamecode/market.h>

#include <modules/autoseed.h>
#include <modules/score.h>
#include <attributes/key.h>

#include <util/attrib.h>
#include <util/base36.h>
#include <util/eventbus.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/quicklist.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/storage.h>

#include <iniparser/iniparser.h>
#include <tolua.h>
#include <lua.h>

#include <time.h>
#include <assert.h>

int
log_lua_error(lua_State * L)
{
  const char* error = lua_tostring(L, -1);

  log_error(("LUA call failed.\n%s\n", error));
  lua_pop(L, 1);

  return 1;
}

int tolua_orderlist_next(lua_State *L)
{
  order** order_ptr = (order **)lua_touserdata(L, lua_upvalueindex(1));
  order* ord = *order_ptr;
  if (ord != NULL) {
    char cmd[8192];
    write_order(ord, cmd, sizeof(cmd));
    tolua_pushstring(L, cmd);
    *order_ptr = ord->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int tolua_quicklist_iter(lua_State *L)
{
  quicklist** qlp = (quicklist **)lua_touserdata(L, lua_upvalueindex(1));
  quicklist* ql = *qlp;
  if (ql != NULL) {
    int index = lua_tointeger(L, lua_upvalueindex(2));
    const char * type = lua_tostring(L, lua_upvalueindex(3));
    void * data = ql_get(ql, index);
    tolua_pushusertype(L, data, TOLUA_CAST type);
    ql_advance(qlp, &index, 1);

    tolua_pushnumber(L, index);
    lua_replace(L, lua_upvalueindex(2));
    return 1;
  }
  else return 0;  /* no more values to return */
}

int tolua_quicklist_push(struct lua_State *L, const char * list_type, const char * elem_type, struct quicklist * list)
{
  if (list) {
    quicklist ** qlist_ptr = (quicklist**)lua_newuserdata(L, sizeof(quicklist *));
    *qlist_ptr = list;

    luaL_getmetatable(L, list_type);
    lua_setmetatable(L, -2);
    lua_pushnumber(L, 0);
    lua_pushstring(L, elem_type);
    lua_pushcclosure(L, tolua_quicklist_iter, 3); /* OBS: this closure has multiple upvalues (list, index, type_name) */
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int tolua_itemlist_next(lua_State *L)
{
  item** item_ptr = (item **)lua_touserdata(L, lua_upvalueindex(1));
  item* itm = *item_ptr;
  if (itm != NULL) {
    tolua_pushstring(L, itm->type->rtype->_name[0]);
    *item_ptr = itm->next;
    return 1;
  }
  else return 0;  /* no more values to return */
}

static int 
tolua_autoseed(lua_State * L)
{
  const char * filename = tolua_tostring(L, 1, 0);
  int new_island = tolua_toboolean(L, 2, 0);
  newfaction * players = read_newfactions(filename);

  if (players!=NULL) {
    while (players) {
      int n = listlen(players);
      int k = (n+ISLANDSIZE-1)/ISLANDSIZE;
      k = n / k;
      n = autoseed(&players, k, new_island?0:TURNS_PER_ISLAND);
      if (n==0) {
        break;
      }
    }
  }
  return 0;
}


static int
tolua_getkey(lua_State* L)
{
  const char * name = tolua_tostring(L, 1, 0);

  int flag = atoi36(name);
  attrib * a = find_key(global.attribs, flag);
  lua_pushboolean(L, a!=NULL);

  return 1;
}

static int
tolua_translate(lua_State* L)
{
  const char * str = tolua_tostring(L, 1, 0);
  const char * lang = tolua_tostring(L, 2, 0);
  struct locale * loc = lang?find_locale(lang):default_locale;
  if (loc) {
    str = locale_string(loc, str);
    tolua_pushstring(L, str);
    return 1;
  }
  return 0;
}

static int
tolua_setkey(lua_State* L)
{
  const char * name = tolua_tostring(L, 1, 0);
  int value = tolua_toboolean(L, 2, 0);

  int flag = atoi36(name);
  attrib * a = find_key(global.attribs, flag);
  if (a==NULL && value) {
    add_key(&global.attribs, flag);
  } else if (a!=NULL && !value) {
    a_remove(&global.attribs, a);
  }

  return 0;
}

static int
tolua_rng_int(lua_State* L)
{
  lua_pushnumber(L, (lua_Number)rng_int());
  return 1;
}

static int
tolua_read_orders(lua_State* L)
{
  const char * filename = tolua_tostring(L, 1, 0);
  int result = readorders(filename);
  lua_pushnumber(L, (lua_Number)result);
  return 1;
}

static int
tolua_message_unit(lua_State* L)
{
  unit * sender = (unit *)tolua_tousertype(L, 1, 0);
  unit * target = (unit *)tolua_tousertype(L, 2, 0);
  const char * str = tolua_tostring(L, 3, 0);
  if (!target) tolua_error(L, TOLUA_CAST "target is nil", NULL);
  if (!sender) tolua_error(L, TOLUA_CAST "sender is nil", NULL);
  deliverMail(target->faction, sender->region, sender, str, target);
  return 0;
}

static int
tolua_message_faction(lua_State * L)
{
  unit * sender = (unit *)tolua_tousertype(L, 1, 0);
  faction * target = (faction *)tolua_tousertype(L, 2, 0);
  const char * str = tolua_tostring(L, 3, 0);
  if (!target) tolua_error(L, TOLUA_CAST "target is nil", NULL);
  if (!sender) tolua_error(L, TOLUA_CAST "sender is nil", NULL);

  deliverMail(target, sender->region, sender, str, NULL);
  return 0;
}

static int
tolua_message_region(lua_State * L)
{
  unit * sender = (unit *)tolua_tousertype(L, 1, 0);
  const char * str = tolua_tostring(L, 2, 0);

  if (!sender) tolua_error(L, TOLUA_CAST "sender is nil", NULL);
  ADDMSG(&sender->region->msgs, msg_message("mail_result", "unit message", sender, str));

  return 0;
}

static int
tolua_update_guards(lua_State * L)
{
  update_guards();
  return 0;
}

static int
tolua_set_turn(lua_State * L)
{
  turn = (int)tolua_tonumber(L, 1, 0);
  return 0;
}

static int
tolua_get_turn(lua_State * L)
{
  tolua_pushnumber(L, (lua_Number)turn);
  return 1;
}

static int
tolua_atoi36(lua_State * L)
{
  const char * s = tolua_tostring(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)atoi36(s));
  return 1;
}

static int
tolua_itoa36(lua_State * L)
{
  int i = (int)tolua_tonumber(L, 1, 0);
  tolua_pushstring(L, itoa36(i));
  return 1;
}

static int
tolua_dice_rand(lua_State * L)
{
  const char * s = tolua_tostring(L, 1, 0);
  tolua_pushnumber(L, dice_rand(s));
  return 1;
}

static int
tolua_addequipment(lua_State * L)
{
  const char * eqname = tolua_tostring(L, 1, 0);
  const char * iname = tolua_tostring(L, 2, 0);
  const char * value = tolua_tostring(L, 3, 0);
  int result = -1;
  if (iname!=NULL) {
    const struct item_type * itype = it_find(iname);
    if (itype!=NULL) { 
      equipment_setitem(create_equipment(eqname), itype, value);
      result = 0;
    }
  }
  lua_pushnumber(L, (lua_Number)result);
  return 1;
}

static int
tolua_get_season(lua_State * L)
{
  int turnno = (int)tolua_tonumber(L, 1, 0);
  gamedate gd;
  get_gamedate(turnno, &gd);
  tolua_pushstring(L, seasonnames[gd.season]);
  return 1;
}

static int
tolua_create_curse(lua_State * L)
{
  unit * u = (unit *)tolua_tousertype(L, 1, 0);
  tolua_Error tolua_err;
  attrib ** ap = NULL;

  if (tolua_isusertype(L, 2, TOLUA_CAST "unit", 0, &tolua_err)) {
    unit * target = (unit *)tolua_tousertype(L, 2, 0);
    if (target) ap = &target->attribs;
  } else if (tolua_isusertype(L, 2, TOLUA_CAST "region", 0, &tolua_err)) {
    region * target = (region *)tolua_tousertype(L, 2, 0);
    if (target) ap = &target->attribs;
  } else if (tolua_isusertype(L, 2, TOLUA_CAST "ship", 0, &tolua_err)) {
    ship * target = (ship *)tolua_tousertype(L, 2, 0);
    if (target) ap = &target->attribs;
  } else if (tolua_isusertype(L, 2, TOLUA_CAST "building", 0, &tolua_err)) {
    building * target = (building *)tolua_tousertype(L, 2, 0);
    if (target) ap = &target->attribs;
  }
  if (ap) {
    const char * cname = tolua_tostring(L, 3, 0);
    const curse_type * ctype = ct_find(cname);
    if (ctype) {
      double vigour = tolua_tonumber(L, 4, 0);
      int duration = (int)tolua_tonumber(L, 5, 0);
      double effect = tolua_tonumber(L, 6, 0);
      int men = (int)tolua_tonumber(L, 7, 0); /* optional */
      curse * c = create_curse(u, ap, ctype, vigour, duration, effect, men);

      if (c) {
        tolua_pushboolean(L, true);
        return 1;
      }
    }
  }
  tolua_pushboolean(L, false);
  return 1;
}

static int
tolua_learn_skill(lua_State * L)
{
  unit * u = (unit *)tolua_tousertype(L, 1, 0);
  const char * skname = tolua_tostring(L, 2, 0);
  float chances = (float)tolua_tonumber(L, 3, 0);
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    learn_skill(u, sk, chances);
  }
  return 0;
}

static int
tolua_update_scores(lua_State * L)
{
  score();
  return 0;
}

static int
tolua_update_owners(lua_State * L)
{
  region * r;
  for (r=regions;r;r=r->next) {
    update_owners(r);
  }
  return 0;
}

static int
tolua_update_subscriptions(lua_State * L)
{
  update_subscriptions();
  return 0;
}

static int 
tolua_remove_empty_units(lua_State * L)
{
  remove_empty_units();
  return 0;
}

static int
tolua_get_nmrs(lua_State * L)
{
  int result = -1;
  int n = (int)tolua_tonumber(L, 1, 0);
  if (n>=0 && n<=NMRTimeout()) {
    if (nmrs==NULL) {
      update_nmrs();
    }
    result = nmrs[n];
  }
  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}

static int
tolua_equipunit(lua_State * L)
{
  unit * u = (unit *)tolua_tousertype(L, 1, 0);
  const char * eqname = tolua_tostring(L, 2, 0);

  equip_unit(u, get_equipment(eqname));

  return 0;
}

static int
tolua_equipment_setitem(lua_State * L)
{
  int result = -1;
  const char * eqname = tolua_tostring(L, 1, 0);
  const char * iname = tolua_tostring(L, 2, 0);
  const char * value = tolua_tostring(L, 3, 0);
  if (iname!=NULL) {
    const struct item_type * itype = it_find(iname);
    if (itype!=NULL) {
      equipment_setitem(create_equipment(eqname), itype, value);
      result = 0;
    }
  }
  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}

#ifdef TODO_FOSS
static int
tolua_levitate_ship(lua_State * L)
{
  ship * sh = (ship *)tolua_tousertype(L, 1, 0);
  unit * mage = (unit *)tolua_tousertype(L, 2, 0);
  double power = (double)tolua_tonumber(L, 3, 0);
  int duration = (int)tolua_tonumber(L, 4, 0);
  int cno = levitate_ship(sh, mage, power, duration);
  tolua_pushnumber(L, (lua_Number)cno);
  return 1;
}
#endif

static int
tolua_spawn_braineaters(lua_State * L)
{
  float chance = (float)tolua_tonumber(L, 1, 0);
  spawn_braineaters(chance);
  return 0;
}

static int
tolua_init_reports(lua_State* L)
{
  int result = init_reports();
  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}

static int
tolua_write_report(lua_State* L)
{
  faction * f = (faction * )tolua_tousertype(L, 1, 0);
  time_t ltime = time(0);
  int result = write_reports(f, ltime);

  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}

static int
tolua_write_reports(lua_State* L)
{
  int result;

  init_reports();
  result = reports();
  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}

static void
reset_game(void)
{
  region * r;
  faction * f;
  for (r=regions;r;r=r->next) {
    unit * u;
    building * b;
    r->flags &= RF_SAVEMASK;
    for (u=r->units;u;u=u->next) {
      u->flags &= UFL_SAVEMASK;
    }
    for (b=r->buildings;b;b=b->next) {
      b->flags &= BLD_SAVEMASK;
    }

    if (r->land && r->land->ownership && r->land->ownership->owner) {
      faction * owner = r->land->ownership->owner;
      if (owner==get_monsters()) {
        /* some compat-fix, i believe. */
        owner = update_owners(r);
      }
      if (owner) {
        fset(r, RF_GUARDED);
      }
    }
  }
  for (f=factions;f;f=f->next) {
    f->flags &= FFL_SAVEMASK;
  }
}

static int
tolua_process_orders(lua_State* L)
{
  ++turn;
  reset_game();
  processorders();
  return 0;
}

static int
tolua_write_passwords(lua_State* L)
{
  int result = writepasswd();
  lua_pushnumber(L, (lua_Number)result);
  return 0;
}

static struct summary * sum_begin = 0;

static int
tolua_init_summary(lua_State* L)
{
  sum_begin = make_summary();
  return 0;
}

static int
tolua_write_summary(lua_State* L)
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
tolua_free_game(lua_State* L)
{
  free_gamedata();
  return 0;
}

static int
tolua_write_map(lua_State* L)
{
  const char * filename = tolua_tostring(L, 1, 0);
  if (filename) {
    crwritemap(filename);
  }
  return 0;
}

static int
tolua_write_game(lua_State* L)
{
  const char * filename = tolua_tostring(L, 1, 0);
  const char * mode =  tolua_tostring(L, 2, 0);

  int result, m = IO_BINARY;
  if (mode && strcmp(mode, "text")==0) m = IO_TEXT;
  remove_empty_factions();
  result = writegame(filename, m);

  tolua_pushnumber(L, (lua_Number)result);
  return 1;
}

static int
tolua_read_game(lua_State* L)
{
  const char * filename = tolua_tostring(L, 1, 0);
  const char * mode =  tolua_tostring(L, 2, 0);

  int rv, m = IO_BINARY;
  if (mode && strcmp(mode, "text")==0) m = IO_TEXT;
  rv = readgame(filename, m, false);

  tolua_pushnumber(L, (lua_Number)rv);
  return 1;
}

static int
tolua_read_turn(lua_State* L)
{
  int cturn = current_turn();
  tolua_pushnumber(L, (lua_Number)cturn);
  return 1;
}

static int
tolua_get_faction(lua_State* L)
{
  int no = tolua_toid(L, 1, 0);
  faction * f = findfaction(no);

  tolua_pushusertype(L, f, TOLUA_CAST "faction");
  return 1;
}

static int
tolua_get_region(lua_State* L)
{
  int x = (int)tolua_tonumber(L, 1, 0);
  int y = (int)tolua_tonumber(L, 2, 0);
  region * r;

  assert(!pnormalize(&x, &y, findplane(x, y)));
  r = findregion(x, y);

  tolua_pushusertype(L, r, TOLUA_CAST "region");
  return 1;
}

static int
tolua_get_region_byid(lua_State* L)
{
  int uid = (int)tolua_tonumber(L, 1, 0);
  region * r = findregionbyid(uid);

  tolua_pushusertype(L, r, TOLUA_CAST "region");
  return 1;
}

static int
tolua_get_building(lua_State* L)
{
  int no = tolua_toid(L, 1, 0);
  building * b = findbuilding(no);

  tolua_pushusertype(L, b, TOLUA_CAST "building");
  return 1;
}

static int
tolua_get_ship(lua_State* L)
{
  int no = tolua_toid(L, 1, 0);
  ship * sh = findship(no);

  tolua_pushusertype(L, sh, TOLUA_CAST "ship");
  return 1;
}

static int
tolua_get_alliance(lua_State* L)
{
  int no = tolua_toid(L, 1, 0);
  alliance * f = findalliance(no);

  tolua_pushusertype(L, f, TOLUA_CAST "alliance");
  return 1;
}

static int
tolua_get_unit(lua_State* L)
{
  int no = tolua_toid(L, 1, 0);
  unit * u = findunit(no);

  tolua_pushusertype(L, u, TOLUA_CAST "unit");
  return 1;
}

static int
tolua_alliance_create(lua_State* L)
{
  int id = (int)tolua_tonumber(L, 1, 0);
  const char * name = tolua_tostring(L, 2, 0);

  alliance * alli = makealliance(id, name);

  tolua_pushusertype(L, alli, TOLUA_CAST "alliance");
  return 1;
}

static int
tolua_get_regions(lua_State* L)
{
  region ** region_ptr = (region**)lua_newuserdata(L, sizeof(region *));

  luaL_getmetatable(L, "region");
  lua_setmetatable(L, -2);

  *region_ptr = regions;

  lua_pushcclosure(L, tolua_regionlist_next, 1);
  return 1;
}

static int
tolua_get_factions(lua_State* L)
{
  faction ** faction_ptr = (faction**)lua_newuserdata(L, sizeof(faction *));

  luaL_getmetatable(L, "faction");
  lua_setmetatable(L, -2);

  *faction_ptr = factions;

  lua_pushcclosure(L, tolua_factionlist_next, 1);
  return 1;
}

static int
tolua_get_alliance_factions(lua_State* L)
{
  alliance * self = (alliance *)tolua_tousertype(L, 1, 0);
  return tolua_quicklist_push(L, "faction_list", "faction", self->members);
}

static int tolua_get_alliance_id(lua_State* L)
{
  alliance* self = (alliance*) tolua_tousertype(L, 1, 0);
  tolua_pushnumber(L, (lua_Number)self->id);
  return 1;
}

static int tolua_get_alliance_name(lua_State* L)
{
  alliance* self = (alliance*) tolua_tousertype(L, 1, 0);
  tolua_pushstring(L, self->name);
  return 1;
}

static int tolua_set_alliance_name(lua_State* L)
{
  alliance* self = (alliance*)tolua_tousertype(L, 1, 0);
  alliance_setname(self, tolua_tostring(L, 2, 0));
  return 0;
}

#include <libxml/tree.h>
#include <util/functions.h>
#include <util/xml.h>
#include <kernel/spell.h>

static int
tolua_write_spells(lua_State* L)
{
  spell_f fun = (spell_f)get_function("lua_castspell");
  const char * filename = "magic.xml";
  xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNodePtr root = xmlNewNode(NULL, BAD_CAST "spells");
  quicklist * ql;
  int qi;

  for (ql=spells,qi=0;ql;ql_advance(&ql, &qi, 1)) {
    spell * sp = (spell *)ql_get(ql, qi);
    if (sp->sp_function!=fun) {
      int combat = 0;
      xmlNodePtr node = xmlNewNode(NULL, BAD_CAST "spell");
      xmlNewProp(node, BAD_CAST "name", BAD_CAST sp->sname);
      xmlNewProp(node, BAD_CAST "type", BAD_CAST magic_school[sp->magietyp]);
      xmlNewProp(node, BAD_CAST "rank", xml_i(sp->rank));
      xmlNewProp(node, BAD_CAST "level", xml_i(sp->level));
      xmlNewProp(node, BAD_CAST "index", xml_i(sp->id));
      if (sp->syntax) xmlNewProp(node, BAD_CAST "syntax", BAD_CAST sp->syntax);
      if (sp->parameter) xmlNewProp(node, BAD_CAST "parameters", BAD_CAST sp->parameter);
      if (sp->components) {
        spell_component * comp = sp->components;
        for (;comp->type!=0;++comp) {
          static const char * costs[] = { "fixed", "level", "linear" };
          xmlNodePtr cnode = xmlNewNode(NULL, BAD_CAST "resource");
          xmlNewProp(cnode, BAD_CAST "name", BAD_CAST comp->type->_name[0]);
          xmlNewProp(cnode, BAD_CAST "amount", xml_i(comp->amount));
          xmlNewProp(cnode, BAD_CAST "cost", BAD_CAST costs[comp->cost]);
          xmlAddChild(node, cnode); 
        }
      }

      if (sp->sptyp & TESTCANSEE) {
        xmlNewProp(node, BAD_CAST "los", BAD_CAST "true");
      }
      if (sp->sptyp & ONSHIPCAST) {
        xmlNewProp(node, BAD_CAST "ship", BAD_CAST "true");
      }
      if (sp->sptyp & OCEANCASTABLE) {
        xmlNewProp(node, BAD_CAST "ocean", BAD_CAST "true");
      }
      if (sp->sptyp & FARCASTING) {
        xmlNewProp(node, BAD_CAST "far", BAD_CAST "true");
      }
      if (sp->sptyp & SPELLLEVEL) {
        xmlNewProp(node, BAD_CAST "variable", BAD_CAST "true");
      }

      if (sp->sptyp & POSTCOMBATSPELL) combat = 3;
      else if (sp->sptyp & COMBATSPELL) combat = 2;
      else if (sp->sptyp & PRECOMBATSPELL) combat = 1;
      if (combat) {
        xmlNewProp(node, BAD_CAST "combat", xml_i(combat));
      }
      xmlAddChild(root, node); 
    }
  }
  xmlDocSetRootElement(doc, root);
  xmlKeepBlanksDefault(0);
  xmlSaveFormatFileEnc(filename, doc, "utf-8", 1);
  xmlFreeDoc(doc);
  return 0;
}

static int
tolua_get_locales(lua_State *L)
{
  const struct locale * lang;
  int i = 0, n = 0;

  for (lang = locales;lang;lang = nextlocale(lang)) ++n;
  lua_createtable(L, n, 0);

  for (lang = locales;lang;lang = nextlocale(lang)) {
    tolua_pushstring(L, TOLUA_CAST locale_name(lang));
    lua_rawseti(L, -2, ++i);
  }
  return 1;
}

static int
tolua_get_spell_text(lua_State *L)
{
  const struct locale * loc = default_locale;
  spell * self = (spell *)tolua_tousertype(L, 1, 0);
  lua_pushstring(L, spell_info(self, loc));
  return 1;
}

static int
tolua_get_spell_school(lua_State *L)
{
  spell * self = (spell *)tolua_tousertype(L, 1, 0);
  lua_pushstring(L, magic_school[self->magietyp]);
  return 1;
}

static int
tolua_get_spell_level(lua_State *L)
{
  spell * self = (spell *)tolua_tousertype(L, 1, 0);
  lua_pushnumber(L, self->level);
  return 1;
}

static int
tolua_get_spell_name(lua_State *L)
{
  const struct locale * lang = default_locale;
  spell * self = (spell *)tolua_tousertype(L, 1, 0);
  lua_pushstring(L, spell_name(self, lang));
  return 1;
}

static int tolua_get_spells(lua_State* L)
{
  return tolua_quicklist_push(L, "spell_list", "spell", spells);
}

int
tolua_read_xml(lua_State* L)
{
  const char * filename = tolua_tostring(L, 1, 0);
  const char * catalog = tolua_tostring(L, 2, 0);
  init_data(filename, catalog);
  return 0;
}

int tolua_process_markets(lua_State* L) {
  do_markets();
  return 0;
}

int tolua_process_produce(lua_State* L) {
  region * r;
  for (r=regions;r;r=r->next) {
    produce(r);
  }
  return 0;
}

typedef struct event_args {
  int hfunction;
  int hargs;
  const char * sendertype;
} event_args;

static void args_free(void * udata)
{
  free(udata);
}

static void event_cb(void * sender, const char * event, void * udata) {
  lua_State * L = (lua_State *)global.vm_state;
  event_args * args = (event_args *)udata;
  int nargs = 2;

  lua_rawgeti(L, LUA_REGISTRYINDEX, args->hfunction);
  if (sender && args->sendertype) {
    tolua_pushusertype(L, sender, TOLUA_CAST args->sendertype);
  } else {
    lua_pushnil(L);
  }
  tolua_pushstring(L, event);
  if (args->hargs) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, args->hfunction);
    ++nargs;
  }
  lua_pcall(L, nargs, 0, 0);
}

static int
tolua_eventbus_register(lua_State * L)
{
  /* parameters:
  **  1: sender (usertype)
  **  2: event (string)
  **  3: handler (function)
  **  4: arguments (any, *optional*)
  */
  void * sender = tolua_tousertype(L, 1, 0);
  const char * event = tolua_tostring(L, 2, 0);
  event_args * args = malloc(sizeof(event_args));

  args->sendertype = sender?tolua_typename(L, 1):NULL;
  lua_pushvalue(L, 3);
  args->hfunction = luaL_ref(L, LUA_REGISTRYINDEX);
  if (lua_type(L, 4)!=LUA_TNONE) {
    lua_pushvalue(L, 4);
    args->hargs = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    args->hargs = 0;
  }
  eventbus_register(sender, event, &event_cb, &args_free, args);
  return 0;
}

static int
tolua_eventbus_fire(lua_State * L)
{
  void * sender = tolua_tousertype(L, 1, 0);
  const char * event = tolua_tostring(L, 2, 0);
  void * args = NULL;
  eventbus_fire(sender, event, args);
  return 0;
}

static int
tolua_report_unit(lua_State* L)
{
  char buffer[512];
  unit * u =  (unit *)tolua_tousertype(L, 1, 0);
  faction * f =  (faction *)tolua_tousertype(L, 2, 0);
  bufunit(f, u, 0, see_unit, buffer, sizeof(buffer));
  tolua_pushstring(L, buffer);
  return 1;
}

static int
tolua_settings_get(lua_State* L)
{
  const char * name = tolua_tostring(L, 1, 0);
  tolua_pushstring(L, get_param(global.parameters, name));
  return 1;
}

static int
tolua_settings_set(lua_State* L)
{
  const char * name = tolua_tostring(L, 1, 0);
  const char * value = tolua_tostring(L, 2, 0);
  set_param(&global.parameters, name, value);
  return 0;
}

static void
parse_inifile(lua_State* L, dictionary * d, const char * section)
{
  int i;
  size_t len = strlen(section);
  for (i=0;d && i!=d->n;++i) {
    const char * key = d->key[i];

    if (strncmp(section, key, len)==0 && key[len]==':') {
      const char * str_value = d->val[i];
      char * endp;
      double num_value = strtod(str_value, &endp);
      lua_pushstring(L, key + len + 1);
      if (*endp) {
        tolua_pushstring(L, str_value);
      } else {
        tolua_pushnumber(L, num_value);
      }
      lua_rawset(L,-3);
    }
  }
  /* special case */
  lua_pushstring(L, "basepath");
  lua_pushstring(L, basepath());
  lua_rawset(L,-3);
}

int
tolua_eressea_open(lua_State* L)
{
  tolua_open(L);

  /* register user types */
  tolua_usertype(L, TOLUA_CAST "spell");
  tolua_usertype(L, TOLUA_CAST "spell_list");
  tolua_usertype(L, TOLUA_CAST "order");
  tolua_usertype(L, TOLUA_CAST "item");
  tolua_usertype(L, TOLUA_CAST "alliance");
  tolua_usertype(L, TOLUA_CAST "event");
  tolua_usertype(L, TOLUA_CAST "faction_list");

  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_module(L, "process", 0);
    tolua_beginmodule(L, "process");
    {
      tolua_function(L, "markets", &tolua_process_markets);
      tolua_function(L, "produce", &tolua_process_produce);
    }
    tolua_endmodule(L);

    tolua_cclass(L, TOLUA_CAST "alliance", TOLUA_CAST "alliance", TOLUA_CAST "", NULL);
    tolua_beginmodule(L, TOLUA_CAST "alliance");
    {
      tolua_variable(L, TOLUA_CAST "name", tolua_get_alliance_name, tolua_set_alliance_name);
      tolua_variable(L, TOLUA_CAST "id", tolua_get_alliance_id, NULL);
      tolua_variable(L, TOLUA_CAST "factions", &tolua_get_alliance_factions, NULL);
      tolua_function(L, TOLUA_CAST "create", tolua_alliance_create);
    }
    tolua_endmodule(L);

    tolua_cclass(L, TOLUA_CAST "spell", TOLUA_CAST "spell", TOLUA_CAST "", NULL);
    tolua_beginmodule(L, TOLUA_CAST "spell");
    {
      tolua_function(L, TOLUA_CAST "__tostring", tolua_get_spell_name);
      tolua_variable(L, TOLUA_CAST "name", tolua_get_spell_name, 0);
      tolua_variable(L, TOLUA_CAST "school", tolua_get_spell_school, 0);
      tolua_variable(L, TOLUA_CAST "level", tolua_get_spell_level, 0);
      tolua_variable(L, TOLUA_CAST "text", tolua_get_spell_text, 0);
    }
    tolua_endmodule(L);

    tolua_module(L, TOLUA_CAST "eventbus", 1);
    tolua_beginmodule(L, TOLUA_CAST "eventbus");
    {
      tolua_function(L, TOLUA_CAST "register", &tolua_eventbus_register);
      tolua_function(L, TOLUA_CAST "fire", &tolua_eventbus_fire);
    }
    tolua_endmodule(L);

    tolua_module(L, TOLUA_CAST "settings", 1);
    tolua_beginmodule(L, TOLUA_CAST "settings");
    {
      tolua_function(L, TOLUA_CAST "set", &tolua_settings_set);
      tolua_function(L, TOLUA_CAST "get", &tolua_settings_get);
    }
    tolua_endmodule(L);

    tolua_module(L, TOLUA_CAST "report", 1);
    tolua_beginmodule(L, TOLUA_CAST "report");
    {
      tolua_function(L, TOLUA_CAST "report_unit", &tolua_report_unit);
    }
    tolua_endmodule(L);

    tolua_module(L, TOLUA_CAST "config", 1);
    tolua_beginmodule(L, TOLUA_CAST "config");
    {
      parse_inifile(L, global.inifile, "config");
      tolua_variable(L, TOLUA_CAST "locales", &tolua_get_locales, 0);
    }
    tolua_endmodule(L);

    tolua_function(L, TOLUA_CAST "get_region_by_id", tolua_get_region_byid);
    tolua_function(L, TOLUA_CAST "get_faction", tolua_get_faction);
    tolua_function(L, TOLUA_CAST "get_unit", tolua_get_unit);
    tolua_function(L, TOLUA_CAST "get_alliance", tolua_get_alliance);
    tolua_function(L, TOLUA_CAST "get_ship", tolua_get_ship),
    tolua_function(L, TOLUA_CAST "get_building", tolua_get_building),
    tolua_function(L, TOLUA_CAST "get_region", tolua_get_region),

    // deprecated_function(L, TOLUA_CAST "add_faction");
    // deprecated_function(L, TOLUA_CAST "faction_origin");
    tolua_function(L, TOLUA_CAST "factions", tolua_get_factions);
    tolua_function(L, TOLUA_CAST "regions", tolua_get_regions);

    tolua_function(L, TOLUA_CAST "read_turn", tolua_read_turn);
    tolua_function(L, TOLUA_CAST "read_game", tolua_read_game);
    tolua_function(L, TOLUA_CAST "write_game", tolua_write_game);
    tolua_function(L, TOLUA_CAST "free_game", tolua_free_game);
    tolua_function(L, TOLUA_CAST "write_map", &tolua_write_map);

    tolua_function(L, TOLUA_CAST "read_orders", tolua_read_orders);
    tolua_function(L, TOLUA_CAST "process_orders", tolua_process_orders);

    tolua_function(L, TOLUA_CAST "init_reports", tolua_init_reports);
    tolua_function(L, TOLUA_CAST "write_reports", tolua_write_reports);
    tolua_function(L, TOLUA_CAST "write_report", tolua_write_report);

    tolua_function(L, TOLUA_CAST "init_summary", tolua_init_summary);
    tolua_function(L, TOLUA_CAST "write_summary", tolua_write_summary);
    tolua_function(L, TOLUA_CAST "write_passwords", tolua_write_passwords),

    tolua_function(L, TOLUA_CAST "message_unit", tolua_message_unit);
    tolua_function(L, TOLUA_CAST "message_faction", tolua_message_faction);
    tolua_function(L, TOLUA_CAST "message_region", tolua_message_region);

    /* scripted monsters */
    tolua_function(L, TOLUA_CAST "spawn_braineaters", tolua_spawn_braineaters);

#ifdef TODO_FOSS
    /* spells and stuff */
    tolua_function(L, TOLUA_CAST "levitate_ship", tolua_levitate_ship);
#endif

    tolua_function(L, TOLUA_CAST "update_guards", tolua_update_guards);

    tolua_function(L, TOLUA_CAST "set_turn", &tolua_set_turn);
    tolua_function(L, TOLUA_CAST "get_turn", &tolua_get_turn);
    tolua_function(L, TOLUA_CAST "get_season", tolua_get_season);

    tolua_function(L, TOLUA_CAST "equipment_setitem", tolua_equipment_setitem);
    tolua_function(L, TOLUA_CAST "equip_unit", tolua_equipunit);
    tolua_function(L, TOLUA_CAST "add_equipment", tolua_addequipment);

    tolua_function(L, TOLUA_CAST "atoi36", tolua_atoi36);
    tolua_function(L, TOLUA_CAST "itoa36", tolua_itoa36);
    tolua_function(L, TOLUA_CAST "dice_roll", tolua_dice_rand);

    tolua_function(L, TOLUA_CAST "get_nmrs", tolua_get_nmrs);
    tolua_function(L, TOLUA_CAST "remove_empty_units", tolua_remove_empty_units);

    tolua_function(L, TOLUA_CAST "update_subscriptions", tolua_update_subscriptions);
    tolua_function(L, TOLUA_CAST "update_scores", tolua_update_scores);
    tolua_function(L, TOLUA_CAST "update_owners", tolua_update_owners);

    tolua_function(L, TOLUA_CAST "learn_skill", tolua_learn_skill);
    tolua_function(L, TOLUA_CAST "create_curse", tolua_create_curse);

    tolua_function(L, TOLUA_CAST "autoseed", tolua_autoseed);

    tolua_function(L, TOLUA_CAST "get_key", tolua_getkey);
    tolua_function(L, TOLUA_CAST "set_key", tolua_setkey);

    tolua_function(L, TOLUA_CAST "translate", &tolua_translate);

    tolua_function(L, TOLUA_CAST "rng_int", tolua_rng_int);

    tolua_function(L, TOLUA_CAST "spells", tolua_get_spells);
    tolua_function(L, TOLUA_CAST "write_spells", tolua_write_spells);

    tolua_function(L, TOLUA_CAST "read_xml", tolua_read_xml);
    
  }
  tolua_endmodule(L);
  return 1;
}
