#include <config.h>
#include <eressea.h>

#include "script.h"
#include "../korrektur.h"

#include <attributes/key.h>
#include <modules/autoseed.h>

// gamecode includes
#include <gamecode/laws.h>
#include <gamecode/monster.h>
#include <gamecode/creport.h>

// kernel includes
#include <kernel/alliance.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/reports.h>
#include <kernel/save.h>
#include <kernel/teleport.h>
#include <kernel/unit.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

// util includes
#include <util/language.h>
#include <util/base36.h>

#include <cstring>
#include <ctime>

using namespace luabind;

static int
lua_addequipment(const char * iname, int number, const char * rcname)
{
  race * rc = rc_find(rcname);
  const struct item_type * itype = it_find(iname);
  if (rc==NULL && strlen(rcname)>0) return -1;
  if (itype==NULL) return -1;
  startup_equipment(itype, number, rc);
  return 0;
}

static int
get_turn(void)
{
  return turn;
}

static int
read_game(const char * filename)
{
  int rv = readgame(filename, false);
  printf(" - Korrekturen Runde %d\n", turn);
  korrektur();
  return rv;
}

static int
write_game(const char *filename)
{
  free_units();
  remove_empty_factions(true);

  return writegame(filename, 0);
}

static summary * sum_begin = 0;

static int
init_summary()
{
  sum_begin = make_summary();
  return 0;
}

static int
write_summary()
{
  if (sum_begin) {
    summary * sum_end = make_summary();
    report_summary(sum_end, sum_begin, false);
    report_summary(sum_end, sum_begin, true);
    return 0;
  }
  return -1;
}
 

extern int process_orders(void);

static int
find_plane_id(const char * name)
{
  plane * pl = getplanebyname(name);
  return pl?pl->id:0;
}

static bool
get_flag(const char * name)
{
  int flag = atoi36(name);
  attrib * a = find_key(global.attribs, flag);
  return (a!=NULL);
}

static void
set_flag(const char * name, bool value)
{
  int flag = atoi36(name);
  attrib * a = find_key(global.attribs, flag);
  if (a==NULL && value) {
    add_key(&global.attribs, flag);
  } else if (a!=NULL && !value) {
    a_remove(&global.attribs, a);
  }
}

static const char *
get_gamename(void)
{
  return global.gamename;
}

static void
lua_setstring(const char * lname, const char * key, const char * str)
{
  struct locale * lang = find_locale(lname);
  locale_setstring(lang, key, str);
}

static const char *
lua_getstring(const char * lname, const char * key)
{
  struct locale * lang = find_locale(lname);
  return locale_getstring(lang, key);
}

static void
lua_planmonsters(void)
{
  unit * u;
  faction * f = findfaction(MONSTER_FACTION);

  if (f==NULL) return;
  if (turn == 0) srand((int)time(0));
  else srand(turn);
  plan_monsters();
  for (u=f->units;u;u=u->nextF) {
    call_script(u);
  }
}

static void 
race_setscript(const char * rcname, const functor<void>& f)
{
  race * rc = rc_find(rcname);
  if (rc!=NULL) {
    luabind::functor<void> * fptr = new luabind::functor<void>(f);
    setscript(&rc->attribs, fptr);
  }
}

#define ISLANDSIZE 20
#define TURNS_PER_ISLAND 3
static void 
lua_autoseed(const char * filename, bool new_island)
{
  newfaction * players = read_newfactions(filename);
  while (players) {
    int n = listlen(players);
    int k = (n+ISLANDSIZE-1)/ISLANDSIZE;
    k = n / k;
    autoseed(&players, k, new_island || (turn % TURNS_PER_ISLAND)==0);
  }
}

#ifdef LUABIND_NO_EXCEPTIONS
static void
error_callback(lua_State * L)
{
}
#endif

static int
get_direction(const char * name)
{
  for (int i=0;i!=MAXDIRECTIONS;++i) {
    if (strcasecmp(directions[i], name)==0) return i;
  }
  return NODIRECTION;
}

int
lua_writereport(faction * f)
{
  time_t ltime = time(0);
  return write_reports(f, ltime);
}

int
lua_writereports(void)
{
  init_reports();
  return reports();
}

void
bind_eressea(lua_State * L)
{
  module(L)[
    def("atoi36", &atoi36),
    def("itoa36", &itoa36),
    def("read_game", &read_game),
    def("write_map", &crwritemap),
    def("write_game", &write_game),
    def("write_passwords", &writepasswd),
    def("init_reports", &init_reports),
    def("write_reports", &lua_writereports),
    def("write_report", &lua_writereport),
    def("init_summary", &init_summary),
    def("write_summary", &write_summary),
    def("read_orders", &readorders),
    def("process_orders", &process_orders),
    def("startup_equipment", &lua_addequipment),
    def("get_turn", &get_turn),
    def("remove_empty_units", &remove_empty_units),

    /* scripted monsters */
    def("plan_monsters", &lua_planmonsters),
    def("spawn_braineaters", &spawn_braineaters),
    def("set_brain", &race_setscript),

    /* map making */
    def("autoseed", lua_autoseed),

    /* string to enum */
    def("direction", &get_direction),

    /* localization: */
    def("set_string", &lua_setstring),
    def("get_string", &lua_getstring),

    def("set_flag", &set_flag),
    def("get_flag", &get_flag),

    def("get_gamename", &get_gamename),
    /* planes not really implemented */
    def("get_plane_id", &find_plane_id)
  ];
#ifdef LUABIND_NO_EXCEPTIONS
  luabind::set_error_callback(error_callback);
#endif
}
