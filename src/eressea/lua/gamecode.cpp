#include <config.h>
#include <eressea.h>

#include "script.h"
#include "../korrektur.h"

#include <attributes/key.h>
#include <modules/autoseed.h>
#include <modules/score.h>

// gamecode includes
#include <gamecode/laws.h>
#include <gamecode/monster.h>
#include <gamecode/creport.h>

// kernel includes
#include <kernel/alliance.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
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
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

// util includes
#include <util/language.h>
#include <util/base36.h>
#include <util/rand.h>
#include <util/rng.h>

#include <cstring>
#include <ctime>

using namespace luabind;

static void
lua_planmonsters(void)
{
  unit * u;
  faction * f = findfaction(MONSTER_FACTION);

  if (f==NULL) return;
  if (turn == 0) rng_init((int)time(0));
  else rng_init(turn);
  plan_monsters();
  for (u=f->units;u;u=u->nextF) {
    call_script(u);
  }
}

#define ISLANDSIZE 20
#define TURNS_PER_ISLAND 4
static void
lua_autoseed(const char * filename, bool new_island)
{
  newfaction * players = read_newfactions(filename);
  if (players!=NULL) {
    rng_init(players->subscription);
    while (players) {
      int n = listlen(players);
      int k = (n+ISLANDSIZE-1)/ISLANDSIZE;
      k = n / k;
      n = autoseed(&players, k, new_island || (turn % TURNS_PER_ISLAND)==0);
      if (n==0) {
        break;
      }
    }
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
lua_equipunit(unit& u, const char * eqname)
{
  equip_unit(&u, get_equipment(eqname));
}

static void
update_subscriptions(void)
{
  FILE * F;
  char zText[MAX_PATH];
  faction * f;
  strcat(strcpy(zText, basepath()), "/subscriptions");
  F = fopen(zText, "r");
  if (F==NULL) {
    log_info((0, "could not open %s.\n", zText));
    return;
  }
  for (;;) {
    char zFaction[5];
    int subscription, fno;
    if (fscanf(F, "%d %s", &subscription, zFaction)<=0) break;
    fno = atoi36(zFaction);
    f = findfaction(fno);
    if (f!=NULL) {
      f->subscription=subscription;
    }
  }
  fclose(F);

  sprintf(zText, "subscriptions.%u", turn);
  F = fopen(zText, "w");
  for (f=factions;f!=NULL;f=f->next) {
    fprintf(F, "%s:%u:%s:%s:%s:%u:\n",
      itoa36(f->no), f->subscription, f->email, f->override,
      dbrace(f->race), f->lastorders);
  }
  fclose(F);
}

static void
message_unit(unit& sender, unit& target, const char * str)
{
  deliverMail(target.faction, sender.region, &sender, str, &target);
}

static void
message_faction(unit& sender, faction& target, const char * str)
{
  deliverMail(&target, sender.region, &sender, str, NULL);
}

static void
message_region(unit& sender, const char * str)
{
  ADDMSG(&sender.region->msgs, msg_message("mail_result", "unit message", &sender, str));
}

static void
lua_learnskill(unit& u, const char * skname, float chances)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    learn_skill(&u, sk, chances);
  }
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

#ifdef SHORTPWDS
static void
readshortpwds()
{
  FILE * F;
  char zText[MAX_PATH];
  sprintf(zText, "%s/%s.%u", basepath(), "shortpwds", turn);

  F = fopen(zText, "r");
  if (F==NULL) {
    log_error(("could not open password file %s", zText));
  } else {
    while (!feof(F)) {
      faction * f;
      char passwd[16], faction[5], email[64];
      fscanf(F, "%s %s %s\n", faction, passwd, email);
      f = findfaction(atoi36(faction));
      if (f!=NULL) {
        shortpwd * pwd = (shortpwd*)malloc(sizeof(shortpwd));
        if (set_email(&pwd->email, email)!=0) {
          log_error(("Invalid email address: %s\n", email));
        }
        pwd->pwd = strdup(passwd);
        pwd->used = false;
        pwd->next = f->shortpwds;
        f->shortpwds = pwd;
      }
    }
    fclose(F);
  }
}
#endif

static int
process_orders(void)
{
  if (turn == 0) rng_init((int)time(0));
  else rng_init(turn);

#ifdef SHORTPWDS
  readshortpwds("passwords");
#endif
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
