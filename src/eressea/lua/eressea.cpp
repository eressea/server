#include <config.h>
#include <cstring>
#include <eressea.h>

// kernel includes
#include <gamecode/laws.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/reports.h>
#include <kernel/save.h>
#include <kernel/unit.h>
#include <util/language.h>
#ifdef ALLIANCES
# include <modules/alliance.h>
#endif

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

extern int writepasswd(void);

static int
write_reports()
{
  reports();
  return 0;
}

extern int process_orders(void);

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
    def("get_turn", &get_turn)
  ];
}
