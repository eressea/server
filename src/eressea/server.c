/* vi: set ts=2:
*
*
* Eressea PB(E)M host Copyright (C) 1998-2003
*   Christian Schlittchen (corwin@amber.kn-bremen.de)
*   Katja Zedel (katze@felidae.kn-bremen.de)
*   Henning Peters (faroul@beyond.kn-bremen.de)
*   Enno Rehling (enno@eressea.de)
*   Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
*
*  based on:
*
* Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
* Atlantis v1.7                    Copyright 1996 by Alex Schröder
*
* This program may not be used, modified or distributed without
* prior permission by the authors of Eressea.
* This program may not be sold or used commercially without prior written
* permission from the authors.
*/

/* config includes */
#include <config.h>
#include <kernel/eressea.h>

#include "console.h"
#include "gmtool.h"

/* initialization - TODO: init in separate module */
#include <attributes/attributes.h>
#include <spells/spells.h>
#include <gamecode/spells.h>
#include <triggers/triggers.h>
#include <items/itemtypes.h>
#include <races/races.h>

/* modules includes */
#include <modules/xmas.h>
#include <modules/gmcmd.h>
#include <modules/infocmd.h>
#if MUSEUM_MODULE
#include <modules/museum.h>
#endif
#include <modules/wormhole.h>
#if ARENA_MODULE
#include <modules/arena.h>
#endif
#if DUNGEON_MODULE
#include <modules/dungeon.h>
#endif

/* gamecode includes */
#include <gamecode/archetype.h>
#include <gamecode/economy.h>
#include <gamecode/items.h>
#include <gamecode/laws.h>
#include <gamecode/creport.h>
#include <gamecode/report.h>
#include <gamecode/xmlreport.h>

/* kernel includes */
#include <kernel/connection.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/names.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spell.h>
#include <kernel/teleport.h>
#include <kernel/unit.h>
#include <kernel/xmlreader.h>
#include <kernel/version.h>


/* util includes */
#include <util/base36.h>
#include <util/language.h>
#include <util/goodies.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/sql.h>
#ifdef MSPACES
# include <util/dl/malloc.h>
#endif

/* external iniparser */
#include <iniparser/iniparser.h>

/* lua includes */
#ifdef BINDINGS_TOLUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "tolua/bindings.h"
#include "tolua/helpers.h"
#include "tolua/bind_unit.h"
#include "tolua/bind_faction.h"
#include "tolua/bind_message.h"
#include "tolua/bind_hashtable.h"
#include "tolua/bind_ship.h"
#include "tolua/bind_building.h"
#include "tolua/bind_region.h"
#include "tolua/bind_gmtool.h"
#include "tolua/bind_storage.h"
#endif // BINDINGS_TOLUA

#ifdef BINDINGS_LUABIND
#include "lua/bindings.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif // _MSC_VER
#include <lua.hpp>
#include <luabind/luabind.hpp>
#ifdef _MSC_VER
#pragma warning (pop)
#endif // _MSC_VER
#include "lua/script.h"
#include <boost/version.hpp>

#endif // BINDINGS_LUABIND

#include <libxml/encoding.h>

/* libc includes */
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <wctype.h>
#include <limits.h>
#include <locale.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER)
# include <crtdbg.h>
#endif

/**
** global variables we are importing from other modules
**/
  extern const char * g_reportdir;
  extern const char * g_datadir;
  extern const char * g_basedir;
  extern const char * g_resourcedir;

  extern boolean nobattle;
  extern boolean nomonsters;
  extern boolean battledebug;

  extern int loadplane;
  extern boolean opt_cr_absolute_coords;


/**
** global variables that we are exporting
**/
static char * orders = NULL;
static int nowrite = 0;
static boolean g_writemap = false;
static boolean g_ignore_errors = false;
static const char * luafile = NULL;
static const char * preload = NULL;
static const char * script_path = "scripts";
static int memdebug = 0;
static int g_console = 1;
#if defined(HAVE_SIGACTION) && defined(HAVE_EXECINFO)
#include <execinfo.h>
#include <signal.h>

static void
report_segfault(int signo, siginfo_t * sinf, void * arg)
{
  void * btrace[50];
  size_t size;
  int fd = fileno(stderr);

  fflush(stdout);
  fputs("\n\nProgram received SIGSEGV, backtrace follows.\n", stderr);
  size = backtrace(btrace, 50);
  backtrace_symbols_fd(btrace, size, fd);
  abort();
}

static int
setup_signal_handler(void)
{
  struct sigaction act;

  act.sa_flags = SA_ONESHOT | SA_SIGINFO;
  act.sa_sigaction = report_segfault;
  sigfillset(&act.sa_mask);
  return sigaction(SIGSEGV, &act, NULL);
}
#else
static int
setup_signal_handler(void)
{
  return 0;
}
#endif

static void
game_init(void)
{
  init_triggers();
  init_xmas();

  reports_init();
  report_init();
  creport_init();
  xmlreport_init();

  debug_language("locales.log");
  register_races();
  register_names();
  register_resources();
  register_buildings();
  register_itemfunctions();
  register_spells();
  register_gcspells();
#if DUNGEON_MODULE
  register_dungeon();
#endif
#if MUSEUM_MODULE
  register_museum();
#endif
#if ARENA_MODULE
  register_arena();
#endif
  register_wormholes();

  register_itemtypes();
  register_xmlreader();
  register_archetypes();
  enable_xml_gamecode();

  init_data(xmlfile);

  init_locales();
  init_archetypes();
  init_attributes();
  init_itemtypes();

  init_gmcmd();
#if INFOCMD_MODULE
  init_info();
#endif
}

static const struct {
  const char * name;
  int (*func)(lua_State *);
} lualibs[] = {
  {"",                luaopen_base},
  {LUA_TABLIBNAME,    luaopen_table},
  {LUA_IOLIBNAME,     luaopen_io},
  {LUA_STRLIBNAME,    luaopen_string},
  {LUA_MATHLIBNAME,   luaopen_math},
/*
  {LUA_LOADLIBNAME,   luaopen_package},
  {LUA_DBLIBNAME,     luaopen_debug},
*/
#if LUA_VERSION_NUM>=501
  {LUA_OSLIBNAME,     luaopen_os},
#endif
  { NULL, NULL }
};

static void
openlibs(lua_State * L)
{
  int i;
  for (i=0;lualibs[i].func;++i) {
    lua_pushcfunction(L, lualibs[i].func);
    lua_pushstring(L, lualibs[i].name);
    lua_call(L, 1, 0);
  }
}

static lua_State *
lua_init(void)
{
  lua_State * L = lua_open();

  openlibs(L);
#ifdef BINDINGS_TOLUA
  register_tolua_helpers();
  tolua_eressea_open(L);
  tolua_sqlite_open(L);
  tolua_unit_open(L);
  tolua_building_open(L);
  tolua_ship_open(L);
  tolua_region_open(L);
  tolua_faction_open(L);
  tolua_unit_open(L);
  tolua_message_open(L);
  tolua_hashtable_open(L);
  tolua_gmtool_open(L);
  tolua_storage_open(L);
#endif

#ifdef BINDINGS_LUABIND
  luabind::open(L);

  bind_objects(L);
  bind_eressea(L);
  bind_script(L);
  bind_spell(L);
  bind_alliance(L);
  bind_region(L);
  bind_item(L);
  bind_faction(L);
  bind_unit(L);
  bind_ship(L);
  bind_building(L);
  bind_event(L);
  bind_message(L);
  bind_gamecode(L);

  bind_gmtool(L);
  bind_test(L);
#endif
  return L;
}

static void
lua_done(lua_State * luaState)
{
  lua_close(luaState);
}

static void
game_done(void)
{
#ifdef CLEANUP_CODE
  /* Diese Routine enfernt allen allokierten Speicher wieder. Das ist nur
   * zum Debugging interessant, wenn man Leak Detection hat, und nach
   * nicht freigegebenem Speicher sucht, der nicht bis zum Ende benötigt
   * wird (temporäre Hilsstrukturen) */

  free_game();

  creport_cleanup();
#ifdef REPORT_FORMAT_NR
  report_cleanup();
#endif
  calendar_cleanup();
#endif
}

#define CRTDBG
#ifdef CRTDBG
void
init_crtdbg(void)
{
#if (defined(_MSC_VER))
  int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  if (memdebug==1) {
    flags |= _CRTDBG_CHECK_ALWAYS_DF; /* expensive */
  } else if (memdebug==2) {
    flags = (flags&0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;
  } else if (memdebug==3) {
    flags = (flags&0x0000FFFF) | _CRTDBG_CHECK_EVERY_128_DF;
  } else if (memdebug==4) {
    flags = (flags&0x0000FFFF) | _CRTDBG_CHECK_EVERY_1024_DF;
  }
  _CrtSetDbgFlag(flags);
#endif
}
#endif

static int
usage(const char * prog, const char * arg)
{
  if (arg) {
    fprintf(stderr, "unknown argument: %s\n\n", arg);
  }
  fprintf(stderr, "Usage: %s [options]\n"
    "-o befehlsdatei  : verarbeitet automatisch die angegebene Befehlsdatei\n"
    "-q               : be less verbose\n"
    "-d datadir       : gibt das datenverzeichnis an\n"
    "-b basedir       : gibt das basisverzeichnis an\n"
    "-r resdir        : gibt das resourceverzeichnis an\n"
    "-t turn          : read this datafile, not the most current one\n"
    "-R reportdir     : gibt das reportverzeichnis an\n"
    "-l path          : specify the base script directory\n"
    "-C               : run in interactive mode\n"
    "-e script        : main lua script\n"
    "--lomem          : keine Messages (RAM sparen)\n"
    "--nobattle       : keine Kämpfe\n"
    "--ignore-errors  : ignore errors in scripts (please don\'t)\n"
    "--nomonsters     : keine monster KI\n"
    "--nodebug        : keine Logfiles für Kämpfe\n"
    "--noreports      : absolut keine Reporte schreiben\n"
    "--debug          : schreibt Debug-Ausgaben in die Datei debug\n"
    "--color          : force curses to use colors even when not detected\n"
    "--nocr           : keine CRs\n"
    "--nonr           : keine Reports\n"
    "--crabsolute     : absolute Koordinaten im CR\n"
    "--help           : help\n", prog);
  return -1;
}

static void
setLuaString(lua_State * luaState, const char * name, const char * value)
{
#if defined(BINDINGS_LUABIND)
  luabind::object g = luabind::globals(luaState);
  g[name] = value;
#elif defined(BINDINGS_TOLUA)
  lua_pushstring(luaState, value);
  lua_setglobal(luaState, name);
#endif // BINDINGS_LUABIND
}

static void
setLuaNumber(lua_State * luaState, const char * name, double value)
{
#if defined(BINDINGS_LUABIND)
  luabind::object g = luabind::globals(luaState);
  g[name] = value;
#elif defined(BINDINGS_TOLUA)
  lua_pushnumber(luaState, (lua_Number)value);
  lua_setglobal(luaState, name);
#endif // BINDINGS_LUABIND
}

static int
read_args(int argc, char **argv, lua_State * luaState)
{
  int i;
  char * c;
  for (i=1;i!=argc;++i) {
    if (argv[i][0]!='-') {
      return usage(argv[0], argv[i]);
    } else if (argv[i][1]=='-') { /* long format */
      if (strcmp(argv[i]+2, "nocr")==0) nocr = true;
      else if (strcmp(argv[i]+2, "nosave")==0) nowrite = true;
      else if (strcmp(argv[i]+2, "noreports")==0) {
        noreports = true;
        nocr = true;
        nocr = true;
      }
      else if (strcmp(argv[i]+2, "xml")==0) xmlfile = argv[++i];
      else if (strcmp(argv[i]+2, "ignore-errors")==0) g_ignore_errors = true;
      else if (strcmp(argv[i]+2, "nonr")==0) nonr = true;
      else if (strcmp(argv[i]+2, "lomem")==0) lomem = true;
      else if (strcmp(argv[i]+2, "nobattle")==0) nobattle = true;
      else if (strcmp(argv[i]+2, "nomonsters")==0) nomonsters = true;
      else if (strcmp(argv[i]+2, "nodebug")==0) battledebug = false;
      else if (strcmp(argv[i]+2, "console")==0) luafile=NULL;
      else if (strcmp(argv[i]+2, "crabsolute")==0) opt_cr_absolute_coords = true;
      else if (strcmp(argv[i]+2, "color")==0) {
        /* force the editor to have colors */
        force_color = 1;
      } else if (strcmp(argv[i]+2, "current")==0) {
        turn = -1;
      }
      else if (strcmp(argv[i]+2, "help")==0)
        return usage(argv[0], NULL);
      else
        return usage(argv[0], argv[i]);
    } else switch(argv[i][1]) {
      case 'C':
        g_console = 1;
        luafile=NULL;
        break;
      case 'R':
        g_reportdir = argv[++i];
        break;
      case 'e':
        g_console = 0;
        luafile = argv[++i];
        break;
      case 'd':
        g_datadir = argv[++i];
        break;
      case 'r':
        g_resourcedir = argv[++i];
        break;
      case 'b':
        g_basedir = argv[++i];
        break;
      case 'i':
        xmlfile = argv[++i];
        break;
      case 't':
        g_console = 0;
        turn = atoi(argv[++i]);
        break;
      case 'q':
        verbosity = 0;
        break;
      case 'o':
        if (i<argc) {
          orders = argv[++i];
        } else {
          return usage(argv[0], argv[i]);
        }
        break;
      case 'v':
        verbosity = atoi(argv[++i]);
        break;
      case 'p':
        loadplane = atoi(argv[++i]);
        break;
      case 's':
        c = argv[++i];
        while (*c && (*c!='=')) ++c;
        if (*c) {
          *c++ = 0;
          setLuaString(luaState, argv[i], c);
        }
        break;
      case 'n':
        c = argv[++i];
        while (*c && (*c!='=')) ++c;
        if (*c) {
          *c++ = 0;
          setLuaNumber(luaState, argv[i], atof(c));
        }
        break;
      case 'l':
        script_path = argv[++i];
        break;
      case 'w':
        g_writemap = true;
        break;
      default:
        usage(argv[0], argv[i]);
    }
  }

  if (orders==NULL) {
    static char orderfile[MAX_PATH];
    sprintf(orderfile, "%s/orders.%d", basepath(), turn);
    orders = orderfile;
  }

  /* add some more variables to the lua globals */
  if (script_path) {
    char str[512];
    sprintf(str, "?;?.lua;%s/?.lua;%s/?", script_path, script_path);
    setLuaString(luaState, "LUA_PATH", str);
  }
  setLuaString(luaState, "datapath", datapath());
  setLuaString(luaState, "scriptpath", script_path);
  setLuaString(luaState, "basepath", basepath());
  setLuaString(luaState, "reportpath", reportpath());
  setLuaString(luaState, "resourcepath", resourcepath());
  setLuaString(luaState, "orderfile", orders);

  return 0;
}

static int
my_lua_error(lua_State * L)
{
  const char* error = lua_tostring(L, -1);

  log_error(("A LUA error occurred: %s\n", error));
  lua_pop(L, 1);
  if (!g_ignore_errors) abort();
  return 1;
}

static dictionary * inifile;
static void
load_inifile(const char * filename)
{
  dictionary * d = iniparser_new(filename);
  if (d) {
    const char * str;
    g_basedir = iniparser_getstring(d, "common:base", g_basedir);
    g_resourcedir = iniparser_getstring(d, "common:res", g_resourcedir);
    xmlfile = iniparser_getstring(d, "common:xml", xmlfile);
    script_path = iniparser_getstring(d, "common:scripts", script_path);
    lomem = iniparser_getint(d, "common:lomem", lomem)?1:0;
    memdebug = iniparser_getint(d, "common:memcheck", memdebug);

    str = iniparser_getstring(d, "common:encoding", NULL);
    if (str) enc_gamedata = xmlParseCharEncoding(str);

    verbosity = iniparser_getint(d, "eressea:verbose", 2);
    sqlpatch = iniparser_getint(d, "eressea:sqlpatch", false);
    battledebug = iniparser_getint(d, "eressea:debug", battledebug)?1:0;

    preload = iniparser_getstring(d, "eressea:preload", preload);
    luafile = iniparser_getstring(d, "eressea:run", luafile);
    g_reportdir = iniparser_getstring(d, "eressea:report", g_reportdir);

    /* editor settings */
    force_color = iniparser_getint(d, "editor:color", force_color);

    str = iniparser_getstring(d, "common:locales", "de,en");
    make_locales(str);
  }
  inifile = d;
}

static void
write_spells(void)
{
  struct locale * loc = find_locale("de");
  FILE * F = fopen("spells.csv", "w");
  spell_list * spl = spells;
  for (;spl;spl=spl->next) {
    const spell * sp = spl->data;
    spell_component * spc = sp->components;
    char components[128];
    components[0]=0;
    for (;spc->type;++spc) {
      strcat(components, LOC(loc, spc->type->_name[0]));
      strcat(components, ",");
    }
    fprintf(F, "%s;%d;%s;%s\n", LOC(loc, mkname("spell", sp->sname)), sp->level, LOC(loc, mkname("school", magic_school[sp->magietyp])), components);
  }
  fclose(F);
}

static void
write_skills(void)
{
  struct locale * loc = find_locale("de");
  FILE * F = fopen("skills.csv", "w");
  race * rc;
  skill_t sk;
  fputs("\"Rasse\",", F);
  for (rc=races;rc;rc = rc->next) {
    if (playerrace(rc)) {
      fprintf(F, "\"%s\",", LOC(loc, mkname("race", rc->_name[0])));
    }
  }
  fputc('\n', F);

  for (sk=0;sk!=MAXSKILLS;++sk) {
    const char * str = skillname(sk, loc);
    if (str) {
      fprintf(F, "\"%s\",", str);
      for (rc=races;rc;rc = rc->next) {
        if (playerrace(rc)) {
          if (rc->bonus[sk]) fprintf(F, "%d,", rc->bonus[sk]);
          else fputc(',', F);
        }
      }
      fputc('\n', F);
    }
  }
  fclose(F);
}

int
main(int argc, char *argv[])
{
  int i;
  char * lc_ctype;
  char * lc_numeric;
  lua_State * L = lua_init();
  static int write_csv = 0;

  setup_signal_handler();

  log_open("eressea.log");

  lc_ctype = setlocale(LC_CTYPE, "");
  lc_numeric = setlocale(LC_NUMERIC, "C");
  assert(towlower(0xC4)==0xE4);
  if (lc_ctype) lc_ctype = strdup(lc_ctype);
  if (lc_numeric) lc_numeric = strdup(lc_numeric);

  global.vm_state = L;
  load_inifile("eressea.ini");
  if (verbosity>=4) {
    printf("\n%s PBEM host\n"
      "Copyright (C) 1996-2005 C. Schlittchen, K. Zedel, E. Rehling, H. Peters.\n\n"
      "Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n\n", global.gamename, version());
  }
  if ((i=read_args(argc, argv, L))!=0) return i;

#ifdef CRTDBG
  init_crtdbg();
#endif

  kernel_init();
  game_init();

  if (write_csv) {
    write_skills();
    write_spells();
  }
  /* run the main script */
  if (preload!=NULL) {
    char * tokens = strdup(preload);
    char * filename = strtok(tokens, ":");
    while (filename) {
      char buf[MAX_PATH];
      if (script_path) {
        sprintf(buf, "%s/%s", script_path, filename);
      }
      else strcpy(buf, filename);
      lua_getglobal(L, "dofile");
      lua_pushstring(L, buf);
      if (lua_pcall(L, 1, 0, 0) != 0) {
        my_lua_error(L);
      }
      filename = strtok(NULL, ":");
    }
  }
  if (g_console) lua_console(L);
  else if (luafile) {
    char buf[MAX_PATH];
    if (script_path) {
      sprintf(buf, "%s/%s", script_path, luafile);
    }
    else strcpy(buf, luafile);
#ifdef BINDINGS_LUABIND
    try {
      luabind::call_function<int>(L, "dofile", buf);
    }
    catch (std::runtime_error& rte) {
      log_error(("%s.\n", rte.what()));
    }
    catch (luabind::error& e) {
      lua_State* L = e.state();
      my_lua_error(L);
    }
#elif defined(BINDINGS_TOLUA)
    lua_getglobal(L, "dofile");
    lua_pushstring(L, buf);
    if (lua_pcall(L, 1, 0, 0) != 0) {
      my_lua_error(L);
    }
#endif
  }
#ifdef MSPACES
  malloc_stats();
#endif
  game_done();
  kernel_done();
  lua_done(L);
  log_close();

  setlocale(LC_CTYPE, lc_ctype);
  setlocale(LC_NUMERIC, lc_numeric);
  free(lc_ctype);
  free(lc_numeric);

  if (inifile) iniparser_free(inifile);

  return 0;
}
