#include <config.h>
#include <kernel/eressea.h>

#include "script.h"
#include "../korrektur.h"

#include <attributes/key.h>
#include <modules/autoseed.h>
#include <modules/score.h>

// gamecode includes
#include <gamecode/laws.h>
#include <gamecode/monster.h>
#include <gamecode/creport.h>
#include <gamecode/report.h>
#include <gamecode/summary.h>

#include <spells/spells.h>

// kernel includes
#include <kernel/alliance.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/ship.h>
#include <kernel/message.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/save.h>
#include <kernel/skill.h>
#include <kernel/teleport.h>
#include <kernel/unit.h>

// lua includes
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

// util includes
#include <util/lists.h>
#include <util/log.h>
#include <util/language.h>
#include <util/base36.h>
#include <util/rand.h>
#include <util/rng.h>

#include <libxml/encoding.h>

#include <cstring>
#include <ctime>

using namespace luabind;

static void
lua_planmonsters(void)
{
  unit * u;
  faction * f = get_monsters();

  if (f==NULL) return;
  plan_monsters();
  for (u=f->units;u;u=u->nextF) {
    call_script(u);
  }
}

#ifdef LUABIND_NO_EXCEPTIONS
static void
error_callback(lua_State * L)
{
}
#endif

static int
lua_writereport(faction * f)
{
  time_t ltime = time(0);
  return write_reports(f, ltime);
}

static int
lua_writereports(void)
{
  init_reports();
  return reports();
}

static void
message_unit(unit * sender, unit * target, const char * str)
{
  deliverMail(target->faction, sender->region, sender, str, target);
}

static void
message_faction(unit * sender, faction * target, const char * str)
{
  deliverMail(target, sender->region, sender, str, NULL);
}

static void
message_region(unit * sender, const char * str)
{
  ADDMSG(&sender->region->msgs, msg_message("mail_result", "unit message", sender, str));
}

static void
set_encoding(const char * str)
{
  enc_gamedata = xmlParseCharEncoding(str);
}

static const char *
get_encoding(void)
{
  return xmlGetCharEncodingName((xmlCharEncoding)enc_gamedata);
}

static int
read_game(const char * filename, const char * mode)
{
  int rv, m = IO_TEXT;
  if (strcmp(mode, "binary")==0) m = IO_BINARY;
  rv = readgame(filename, m, false);
  if (rv==0) {
    log_printf(" - Korrekturen Runde %d\n", turn);
    korrektur();
  }
  return rv;
}

static int
write_game(const char *filename, const char * mode)
{
  int result, m = IO_TEXT;
  if (strcmp(mode, "binary")==0) m = IO_BINARY;
  remove_empty_factions(true);
  result = writegame(filename, m);
  return result;
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
  assert(sum_begin
    || !"init_summary must be called before before write_summary");
  if (sum_begin) {
    summary * sum_end = make_summary();
    report_summary(sum_end, sum_begin, false);
    report_summary(sum_end, sum_begin, true);
    return 0;
  }
  return -1;
}


static int
process_orders(void)
{
  turn++;
  processorders();

  return 0;
}

void
bind_gamecode(lua_State * L)
{
  module(L)[
    def("read_game", &read_game),
    def("write_game", &write_game),
    def("free_game", &free_gamedata),

    def("get_encoding", &get_encoding),
    def("set_encoding", &set_encoding),

    def("init_summary", &init_summary),
    def("write_summary", &write_summary),

    def("read_orders", &readorders),
    def("process_orders", &process_orders),

    def("write_map", &crwritemap),
    def("write_passwords", &writepasswd),
    def("init_reports", &init_reports),
    def("write_reports", &lua_writereports),
    def("write_report", &lua_writereport),
    def("update_guards", &update_guards),

    def("message_unit", &message_unit),
    def("message_faction", &message_faction),
    def("message_region", &message_region),

    /* spells and stuff */
    def("levitate_ship", &levitate_ship),

    /* scripted monsters */
    def("spawn_braineaters", &spawn_braineaters),
    def("spawn_undead", &spawn_undead),
    def("spawn_dragons", &spawn_dragons),
    def("plan_monsters", &lua_planmonsters)
  ];
#ifdef LUABIND_NO_EXCEPTIONS
  luabind::set_error_callback(error_callback);
#endif
}
