#include <config.h>
#include <cstring>
#include <eressea.h>

// kernel includes
#include <gamecode/laws.h>
#include <kernel/save.h>
#include <kernel/unit.h>
#include <kernel/reports.h>
#include <util/language.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static int
get_turn(void)
{
  return turn;
}

static int
read_game(void)
{
  return readgame(false);
}

static int
write_game(void)
{
  char ztext[64];

  free_units();
  remove_empty_factions(true);

  sprintf(ztext, "%s/%d", datapath(), turn);
  writegame(ztext, 0);
  return 0;
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
    def("read_game", &read_game),
    def("write_game", &write_game),
    def("write_passwords", &writepasswd),
    def("write_reports", &write_reports),
    def("read_orders", &readorders),
    def("process_orders", &process_orders),
    def("get_turn", &get_turn)
  ];
}
