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
#include <platform.h>
#include <kernel/config.h>

#include <gmtool.h>
#include <eressea.h>

/* initialization - TODO: init in separate module */
#include <attributes/attributes.h>
#include <triggers/triggers.h>
#include <items/itemtypes.h>
#include <races/races.h>

/* modules includes */
#include <modules/xmas.h>
#include <modules/gmcmd.h>
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
#include <gamecode/curses.h>
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
#include <util/console.h>
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
#include <bindings/bindings.h>
#include <bindings/helpers.h>
#include <bindings/bind_attrib.h>
#include <bindings/bind_building.h>
#include <bindings/bind_faction.h>
#include <bindings/bind_gmtool.h>
#include <bindings/bind_hashtable.h>
#include <bindings/bind_message.h>
#include <bindings/bind_region.h>
#include <bindings/bind_ship.h>
#include <bindings/bind_storage.h>
#include <bindings/bind_unit.h>
#endif // BINDINGS_TOLUA

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
static const char * luafile = "init.lua";
static const char * entry_point = NULL;
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
#if defined(BINDINGS_TOLUA)
  lua_pushstring(luaState, value);
  lua_setglobal(luaState, name);
#endif
}

static void
setLuaNumber(lua_State * luaState, const char * name, double value)
{
#if defined(BINDINGS_TOLUA)
  lua_pushnumber(luaState, (lua_Number)value);
  lua_setglobal(luaState, name);
#endif
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
      else if (strcmp(argv[i]+2, "version")==0) {
        printf("\n%s PBEM host\n"
          "Copyright (C) 1996-2005 C. Schlittchen, K. Zedel, E. Rehling, H. Peters.\n\n"
          "Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n\n", global.gamename, version());
      }
      else if (strcmp(argv[i]+2, "xml")==0) game_name = argv[++i];
      else if (strcmp(argv[i]+2, "ignore-errors")==0) g_ignore_errors = true;
      else if (strcmp(argv[i]+2, "nonr")==0) nonr = true;
      else if (strcmp(argv[i]+2, "lomem")==0) lomem = true;
      else if (strcmp(argv[i]+2, "nobattle")==0) nobattle = true;
      else if (strcmp(argv[i]+2, "nomonsters")==0) nomonsters = true;
      else if (strcmp(argv[i]+2, "nodebug")==0) battledebug = false;
      else if (strcmp(argv[i]+2, "console")==0) entry_point=NULL;
      else if (strcmp(argv[i]+2, "crabsolute")==0) opt_cr_absolute_coords = true;
      else if (strcmp(argv[i]+2, "color")==0) {
        /* force the editor to have colors */
        force_color = 1;
      }
      else if (strcmp(argv[i]+2, "help")==0)
        return usage(argv[0], NULL);
      else
        return usage(argv[0], argv[i]);
    } else switch(argv[i][1]) {
      case 'C':
        g_console = 1;
        entry_point = NULL;
        break;
      case 'R':
        g_reportdir = argv[++i];
        break;
      case 'e':
        g_console = 0;
        entry_point = argv[++i];
        break;
      case 'd':
        g_datadir = argv[++i];
        break;
      case 'b':
        g_basedir = argv[++i];
        break;
      case 'i':
        game_name = argv[++i];
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
      case 'w':
        g_writemap = true;
        break;
      default:
        usage(argv[0], argv[i]);
    }
  }

  if (orders!=NULL) {
    setLuaString(luaState, "orderfile", orders);
  }

  setLuaString(luaState, "basepath", basepath());
  setLuaString(luaState, "reportpath", reportpath());

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

static void
load_inifile(const char * filename)
{
  dictionary * d = iniparser_new(filename);
  if (d) {
    const char * str;

    g_basedir = iniparser_getstring(d, "eressea:base", g_basedir);
    lomem = iniparser_getint(d, "eressea:lomem", lomem)?1:0;
    memdebug = iniparser_getint(d, "eressea:memcheck", memdebug);

    str = iniparser_getstring(d, "eressea:encoding", NULL);
    if (str) enc_gamedata = xmlParseCharEncoding(str);

    verbosity = iniparser_getint(d, "eressea:verbose", 2);
    sqlpatch = iniparser_getint(d, "eressea:sqlpatch", false);
    battledebug = iniparser_getint(d, "eressea:debug", battledebug)?1:0;

    entry_point = iniparser_getstring(d, "eressea:run", entry_point);
    luafile = iniparser_getstring(d, "eressea:load", luafile);
    g_reportdir = iniparser_getstring(d, "eressea:report", g_reportdir);

    str = iniparser_getstring(d, "eressea:locales", "de,en");
    make_locales(str);

    /* only one value in the [editor] section */
    force_color = iniparser_getint(d, "editor:color", force_color);

    /* excerpt from [config] (the rest is used in bindings.c) */
    game_name = iniparser_getstring(d, "config:game", game_name);
  }
  global.inifile = d;
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

void locale_init(void)
{
  setlocale(LC_CTYPE, "");
  setlocale(LC_NUMERIC, "C");
  assert(towlower(0xC4)==0xE4); /* &Auml; => &auml; */
}

int
main(int argc, char *argv[])
{
  int i;
  lua_State * L;
  static int write_csv = 0;

  setup_signal_handler();

  log_open("eressea.log");
  locale_init();
  load_inifile("eressea.ini");
  L = lua_init();
  global.vm_state = L;
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
  if (luafile) {
    char buf[MAX_PATH];
    strcpy(buf, luafile);
    lua_getglobal(L, "dofile");
    lua_pushstring(L, buf);
    if (lua_pcall(L, 1, 0, 0) != 0) {
      my_lua_error(L);
    }
  }
  if (entry_point) {
    lua_getglobal(L, entry_point);
    if (lua_pcall(L, 0, 1, 0) != 0) {
      my_lua_error(L);
    }
  } else {
    lua_console(L);
  }
#ifdef MSPACES
  malloc_stats();
#endif
  game_done();
  kernel_done();
  lua_done(L);
  log_close();

  if (global.inifile) iniparser_free(global.inifile);

  return 0;
}
