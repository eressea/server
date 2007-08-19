/* vi: set ts=2:
*
*
* Eressea PB(E)M host Copyright (C) 1998-2003
*   Christian Schlittchen (corwin@amber.kn-bremen.de)
*   Katja Zedel (katze@felidae.kn-bremen.de)
*   Henning Peters (faroul@beyond.kn-bremen.de)
*   Enno Rehling (enno@eressea-pbem.de)
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

#define LOCALE_CHECK
#ifdef __LCC__
#undef LOCALE_CHECK
#endif

/* config includes */
#include <config.h>
#include <eressea.h>

#include "console.h"

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
#ifdef MUSEUM_MODULE
#include <modules/museum.h>
#endif
#ifdef WORMHOLE_MODULE
#include <modules/wormhole.h>
#endif
#ifdef ARENA_MODULE
#include <modules/arena.h>
#endif
#ifdef DUNGEON_MODULE
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
#include <kernel/border.h>
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
#include "lua/bindings.h"
#include "lua/script.h"
#include <boost/version.hpp>
#include <lua.hpp>
#include <luabind/luabind.hpp>

#include <libxml/encoding.h>

/* stdc++ includes */
#include <stdexcept>
#include <string>
#include <sstream>

/* libc includes */
#include <cstdio>
#include <cctype>
#include <climits>
#include <clocale>
#include <cstring>
#include <ctime>

#if defined(USE_DMALLOC)
# define CLEANUP_CODE
#endif

/**
** global variables we are importing from other modules
**/
extern "C" {
  extern const char * g_reportdir;
  extern const char * g_datadir;
  extern const char * g_basedir;
  extern const char * g_resourcedir;

  extern boolean nobattle;
  extern boolean nomonsters;
  extern boolean battledebug;
  extern boolean dirtyload;

  extern int loadplane;

  extern void debug_messagetypes(FILE * out);
  extern void free_region(region * r);
  extern void render_init(void);
  extern void free_borders(void);
  extern boolean opt_cr_absolute_coords;

}

/**
** global variables that we are exporting
**/
static char * orders = NULL;
static int nowrite = 0;
static boolean g_writemap = false;
static boolean g_ignore_errors = false;
static boolean opt_reportonly = false;
static const char * luafile = NULL;
static const char * script_path = "scripts";
static int memdebug = 0;

struct settings global = {
  "Eressea", /* gamename */
  1000, /* maxunits */
};

#ifdef __GNUC__
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
  register_ships();
  register_itemfunctions();
  register_spells();
  register_gcspells();
#ifdef DUNGEON_MODULE
  register_dungeon();
#endif
#ifdef MUSEUM_MODULE
  register_museum();
#endif
#ifdef ARENA_MODULE
  register_arena();
#endif
#ifdef WORMHOLE_MODULE
  register_wormholes();
#endif

  register_itemtypes();
  register_xmlreader();
  register_archetypes();
  enable_xml_gamecode();

  init_data(xmlfile);

  init_locales();
  /*  init_resources(); must be done inside the xml-read, because requirements use items */

  init_archetypes();
  init_attributes();
  init_itemtypes();
  init_rawmaterials();

  init_gmcmd();
#ifdef INFOCMD_MODULE
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
  lua_State * luaState = lua_open();
/*
  luaopen_base(luaState);
  luaopen_math(luaState);
  luaopen_string(luaState);
  luaopen_io(luaState);
  luaopen_table(luaState);
  luaL_openlibs(luaState);
 */
  openlibs(luaState);
  luabind::open(luaState);
  bind_objects(luaState);
  bind_eressea(luaState);
  bind_script(luaState);
  bind_spell(luaState);
  bind_alliance(luaState);
  bind_region(luaState);
  bind_item(luaState);
  bind_faction(luaState);
  bind_unit(luaState);
  bind_ship(luaState);
  bind_building(luaState);
  bind_event(luaState);
  bind_message(luaState);
  bind_gamecode(luaState);
  return luaState;
}

static void
lua_done(lua_State * luaState)
{
  reset_scripts();
  lua_close(luaState);
}

#ifndef CLEANUP_CODE
# define CLEANUP_CODE
#endif
#ifdef CLEANUP_CODE
static void
game_done(void)
{
  /* Diese Routine enfernt allen allokierten Speicher wieder. Das ist nur
  * zum Debugging interessant, wenn man Leak Detection hat, und nach
  * nicht freigegebenem Speicher sucht, der nicht bis zum Ende benötigt
  * wird (temporäre Hilsstrukturen) */

  while (regions) {
    region *r = regions;
    regions = r->next;

    while (r->units) {
      unit * u = r->units;
      r->units = u->next;
      stripunit(u);
      uunhash(u);
      free(u);
    }

    while (r->buildings) {
      building * b = r->buildings;
      r->buildings = b->next;
      free(b->name);
      free(b->display);
      free(b);
    }

    while (r->ships) {
      ship * s = r->ships;
      r->ships = s->next;
      free(s->name);
      free(s->display);
      free(s);
    }
    free_region(r);
  }

  while (factions) {
    faction * f = factions;
    factions = f->next;
    stripfaction(f);
    free(f);
  }

  while (planes) {
    plane * pl = planes;
    planes = planes->next;
    free(pl->name);
    free(pl);
  }

  creport_cleanup();
#ifdef REPORT_FORMAT_NR
  report_cleanup();
#endif
  calendar_cleanup();
}
#endif

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
    "-x n             : Lädt nur die ersten n regionen\n"
    "-f x y           : Lädt nur die regionen ab (x,y)\n"
    "-v befehlsdatei  : verarbeitet automatisch die angegebene Befehlsdatei\n"
    "-q               : be less verbose\n"
    "-d datadir       : gibt das datenverzeichnis an\n"
    "-b basedir       : gibt das basisverzeichnis an\n"
    "-r resdir        : gibt das resourceverzeichnis an\n"
    "-t turn          : read this datafile, not the most current one\n"
    "-o reportdir     : gibt das reportverzeichnis an\n"
    "-l path          : specify the base script directory\n"
    "-C               : run in interactive mode\n"
    "-e script        : main lua script\n"
    "-R               : erstellt nur die Reports neu\n"
    "--lomem          : keine Messages (RAM sparen)\n"
    "--nobattle       : keine Kämpfe\n"
    "--ignore-errors  : ignore errors in scripts (please don\'t)\n"
    "--nomonsters     : keine monster KI\n"
    "--nodebug        : keine Logfiles für Kämpfe\n"
    "--noreports      : absolut keine Reporte schreiben\n"
    "--debug          : schreibt Debug-Ausgaben in die Datei debug\n"
    "--nocr           : keine CRs\n"
    "--nonr           : keine Reports\n"
    "--crabsolute     : absolute Koordinaten im CR\n"
    "--help           : help\n", prog);
  return -1;
}

static void
setLuaString(lua_State * luaState, const char * name, const char * value)
{
#if LUABIND_BETA<7 || (LUABIND_BETA==7 && LUABIND_DEVEL<2)
  luabind::object g = luabind::get_globals(luaState);
#else
  luabind::object g = luabind::globals(luaState);
#endif
  g[name] = value;
}

static void
setLuaNumber(lua_State * luaState, const char * name, double value)
{
#if LUABIND_BETA<7 || (LUABIND_BETA==7 && LUABIND_DEVEL<2)
  luabind::object g = luabind::get_globals(luaState);
#else
  luabind::object g = luabind::globals(luaState);
#endif
  g[name] = value;
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
      else if (strcmp(argv[i]+2, "dirtyload")==0) dirtyload = true;
      else if (strcmp(argv[i]+2, "nonr")==0) nonr = true;
      else if (strcmp(argv[i]+2, "lomem")==0) lomem = true;
      else if (strcmp(argv[i]+2, "nobattle")==0) nobattle = true;
      else if (strcmp(argv[i]+2, "nomonsters")==0) nomonsters = true;
      else if (strcmp(argv[i]+2, "nodebug")==0) battledebug = false;
      else if (strcmp(argv[i]+2, "console")==0) luafile=NULL;
      else if (strcmp(argv[i]+2, "crabsolute")==0) opt_cr_absolute_coords = true;
      else if (strcmp(argv[i]+2, "help")==0)
        return usage(argv[0], NULL);
      else
        return usage(argv[0], argv[i]);
    } else switch(argv[i][1]) {
      case 'C':
        luafile=NULL;
        break;
      case 'o':
        g_reportdir = argv[++i];
        break;
      case 'e':
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
        turn = atoi(argv[++i]);
        break;
      case 'f':
        firstx = atoi(argv[++i]);
        firsty = atoi(argv[++i]);
        break;
      case 'q':
        quiet = 1;
        break;
      case 'v':
        if (i<argc) {
          orders = argv[++i];
        } else {
          return usage(argv[0], argv[i]);
        }
        break;
      case 'p':
        loadplane = atoi(argv[++i]);
        break;
      case 'x':
        maxregions = atoi(argv[++i]);
        maxregions = (maxregions*81+80) / 81;
        break;
      case 'X':
        dirtyload = true;
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
      case 'R':
        opt_reportonly = true;
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

  log_error(("A LUA error occured: %s\n", error));
  lua_pop(L, 1);
  if (!g_ignore_errors) std::terminate();
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

    quiet = iniparser_getint(d, "eressea:verbose", 0)?0:1;
    battledebug = iniparser_getint(d, "eressea:debug", battledebug)?1:0;

    luafile = iniparser_getstring(d, "eressea:run", luafile);
    g_reportdir = iniparser_getstring(d, "eressea:report", g_reportdir);

    str = iniparser_getstring(d, "common:locales", "de");
    make_locales(str);
  }
  inifile = d;
}

int
main(int argc, char *argv[])
{
  int i;
  char * lc_ctype;
  char * lc_numeric;

  setup_signal_handler();

  global.data_version = RELEASE_VERSION;
  sqlpatch = true;
  log_open("eressea.log");
  printf("\n%s PBEM host\n"
    "Copyright (C) 1996-2005 C. Schlittchen, K. Zedel, E. Rehling, H. Peters.\n\n"
    "Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n\n", global.gamename, version());

  lc_ctype = setlocale(LC_CTYPE, "");
  lc_numeric = setlocale(LC_NUMERIC, "C");
  if (lc_ctype) lc_ctype = strdup(lc_ctype);
  if (lc_numeric) lc_numeric = strdup(lc_numeric);
#ifdef LOCALE_CHECK
  if (!locale_check()) {
    log_error(("The current locale is not suitable for international Eressea.\n"));
    return -1;
  }
#endif
  
  lua_State * luaState = lua_init();
  global.vm_state = luaState;
  load_inifile("eressea.ini");
  if ((i=read_args(argc, argv, luaState))!=0) return i;

#ifdef CRTDBG
  init_crtdbg();
#endif

  kernel_init();
  game_init();

  // run the main script
  if (luafile==NULL) lua_console(luaState);
  else {
    char buf[MAX_PATH];
    if (script_path) sprintf(buf, "%s/%s", script_path, luafile);
    else strcpy(buf, luafile);
#ifdef LUABIND_NO_EXCEPTIONS
    luabind::set_error_callback(my_lua_error);
    luabind::set_pcall_callback(my_lua_error);
#else
    try {
#endif
      luabind::call_function<int>(luaState, "dofile", buf);
#ifndef LUABIND_NO_EXCEPTIONS
    } 
    catch (std::runtime_error& rte) {
      log_error(("%s.\n", rte.what()));
    }
    catch (luabind::error& e) {
      lua_State* L = e.state();
      my_lua_error(L);
    }
#endif
  }
#ifdef MSPACES
  malloc_stats();
#endif
#ifdef CLEANUP_CODE
  game_done();
#endif
  kernel_done();
  lua_done(luaState);
  log_close();

  setlocale(LC_CTYPE, lc_ctype);
  setlocale(LC_NUMERIC, lc_numeric);
  free(lc_ctype);
  free(lc_numeric);

  if (inifile) iniparser_free(inifile);

  return 0;
}
