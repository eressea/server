#include <config.h>
#include <cstring>
#include <eressea.h>

// kernel includes
#ifdef ALLIANCES
# include <modules/alliance.h>
#endif
#include <attributes/key.h>
#include <gamecode/laws.h>
#include <kernel/race.h>
#include <kernel/plane.h>
#include <kernel/item.h>
#include <kernel/reports.h>
#include <kernel/save.h>
#include <kernel/unit.h>
#include <util/language.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

// util includes
#include <util/base36.h>

using namespace luabind;

static int
lua_addequipment(const char * iname, int number)
{
  const struct item_type * itype = it_find(iname);
  if (itype==NULL) return -1;
  add_equipment(itype, number);
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
  return readgame(filename, false);
}

static int
write_game(const char *filename)
{
  free_units();
  remove_empty_factions(true);

  return writegame(filename, 0);
}

static int
write_reports()
{
  reports();
  return 0;
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

void
bind_eressea(lua_State * L)
{
  module(L)[
    def("atoi36", &atoi36),
    def("itoa36", &itoa36),
    def("read_game", &read_game),
    def("write_game", &write_game),
    def("write_passwords", &writepasswd),
    def("write_reports", &write_reports),
    def("read_orders", &readorders),
    def("process_orders", &process_orders),
    def("add_equipment", &lua_addequipment),
    def("get_turn", &get_turn),
    def("remove_empty_units", &remove_empty_units),

    def("set_flag", &set_flag),
    def("get_flag", &get_flag),

    def("get_gamename", &get_gamename),
    /* planes not really implemented */
    def("get_plane_id", &find_plane_id)
  ];
}
