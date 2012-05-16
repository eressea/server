#include <platform.h>
#include <util/log.h>

#include <eressea.h>
#include <gmtool.h>
#include <kernel/config.h>
#include <kernel/save.h>
#include <iniparser/iniparser.h>
#include "races/races.h"

#include <assert.h>
#include <locale.h>
#include <wctype.h>

#include <tests.h>

static const char *luafile = "setup.lua";
static const char *entry_point = NULL;
static const char *inifile = "eressea.ini";
static int memdebug = 0;

static void parse_config(const char *filename)
{
  dictionary *d = iniparser_new(filename);
  if (d) {
    load_inifile(d);

    memdebug = iniparser_getint(d, "eressea:memcheck", memdebug);
    entry_point = iniparser_getstring(d, "eressea:run", entry_point);
    luafile = iniparser_getstring(d, "eressea:load", luafile);

    /* only one value in the [editor] section */
    force_color = iniparser_getint(d, "editor:color", force_color);

    /* excerpt from [config] (the rest is used in bindings.c) */
    game_name = iniparser_getstring(d, "config:game", game_name);
  }
  global.inifile = d;
}

static int usage(const char *prog, const char *arg)
{
  if (arg) {
    fprintf(stderr, "unknown argument: %s\n\n", arg);
  }
  fprintf(stderr, "Usage: %s [options]\n"
    "-t <turn>        : read this datafile, not the most current one\n"
    "-q               : be quite (same as -v 0)\n"
    "-v <level>       : verbosity level\n"
    "-C               : run in interactive mode\n"
    "--color          : force curses to use colors even when not detected\n"
    "--tests          : run testsuite\n" "--help           : help\n", prog);
  return -1;
}

static int parse_args(int argc, char **argv, int *exitcode)
{
  int i;
  int run_tests = 0;

  for (i = 1; i != argc; ++i) {
    if (argv[i][0] != '-') {
      return usage(argv[0], argv[i]);
    } else if (argv[i][1] == '-') {     /* long format */
      if (strcmp(argv[i] + 2, "version") == 0) {
        printf("\n%s PBEM host\n"
          "Copyright (C) 1996-2005 C. Schlittchen, K. Zedel, E. Rehling, H. Peters.\n\n"
          "Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n\n",
          global.gamename, version());
      } else if (strcmp(argv[i] + 2, "color") == 0) {
        /* force the editor to have colors */
        force_color = 1;
      } else if (strcmp(argv[i] + 2, "tests") == 0) {
        /* force the editor to have colors */
        run_tests = 1;
      } else if (strcmp(argv[i] + 2, "help") == 0) {
        return usage(argv[0], NULL);
      } else {
        return usage(argv[0], argv[i]);
      }
    } else
      switch (argv[i][1]) {
        case 'C':
          entry_point = NULL;
          break;
        case 'e':
          entry_point = argv[++i];
          break;
        case 't':
          turn = atoi(argv[++i]);
          break;
        case 'q':
          verbosity = 0;
          break;
        case 'v':
          verbosity = atoi(argv[++i]);
          break;
        case 'h':
          usage(argv[0], NULL);
          return 1;
        default:
          *exitcode = -1;
          usage(argv[0], argv[i]);
          return 1;
      }
  }

  switch (verbosity) {
  case 0:
    log_stderr = 0;
    break;
  case 1:
    log_stderr = LOG_CPERROR;
    break;
  case 2:
    log_stderr = LOG_CPERROR|LOG_CPWARNING;
    break;
  case 3:
    log_stderr = LOG_CPERROR|LOG_CPWARNING|LOG_CPDEBUG;
    break;
  default:
    log_stderr = LOG_CPERROR|LOG_CPWARNING|LOG_CPDEBUG|LOG_CPINFO;
    break;
  }
  if (run_tests) {
    *exitcode = RunAllTests();
    return 1;
  }

  return 0;
}

#if defined(HAVE_SIGACTION) && defined(HAVE_EXECINFO)
#include <execinfo.h>
#include <signal.h>

static void report_segfault(int signo, siginfo_t * sinf, void *arg)
{
  void *btrace[50];
  size_t size;
  int fd = fileno(stderr);

  fflush(stdout);
  fputs("\n\nProgram received SIGSEGV, backtrace follows.\n", stderr);
  size = backtrace(btrace, 50);
  backtrace_symbols_fd(btrace, size, fd);
  abort();
}

static int setup_signal_handler(void)
{
  struct sigaction act;

  act.sa_flags = SA_ONESHOT | SA_SIGINFO;
  act.sa_sigaction = report_segfault;
  sigfillset(&act.sa_mask);
  return sigaction(SIGSEGV, &act, NULL);
}
#else
static int setup_signal_handler(void)
{
  return 0;
}
#endif

#undef CRTDBG
#ifdef CRTDBG
#include <crtdbg.h>
void init_crtdbg(void)
{
#if (defined(_MSC_VER))
  int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  if (memdebug == 1) {
    flags |= _CRTDBG_CHECK_ALWAYS_DF;   /* expensive */
  } else if (memdebug == 2) {
    flags = (flags & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;
  } else if (memdebug == 3) {
    flags = (flags & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_128_DF;
  } else if (memdebug == 4) {
    flags = (flags & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_1024_DF;
  }
  _CrtSetDbgFlag(flags);
#endif
}
#endif

static void dump_spells(void)
{
  struct locale *loc = find_locale("de");
  FILE *F = fopen("spells.csv", "w");
  quicklist *ql;
  int qi;

  for (ql = spells, qi = 0; ql; ql_advance(&ql, &qi, 1)) {
    spell *sp = (spell *) ql_get(ql, qi);
    spell_component *spc = sp->components;
    char components[128];
    components[0] = 0;
    for (; spc->type; ++spc) {
      strcat(components, LOC(loc, spc->type->_name[0]));
      strcat(components, ",");
    }
    fprintf(F, "%s;%d;%s;%s\n", LOC(loc, mkname("spell", sp->sname)), sp->level,
      LOC(loc, mkname("school", magic_school[sp->magietyp])), components);
  }
  fclose(F);
}

static void dump_skills(void)
{
  struct locale *loc = find_locale("de");
  FILE *F = fopen("skills.csv", "w");
  race *rc;
  skill_t sk;
  fputs("\"Rasse\",", F);
  for (rc = races; rc; rc = rc->next) {
    if (playerrace(rc)) {
      fprintf(F, "\"%s\",", LOC(loc, mkname("race", rc->_name[0])));
    }
  }
  fputc('\n', F);

  for (sk = 0; sk != MAXSKILLS; ++sk) {
    const char *str = skillname(sk, loc);
    if (str) {
      fprintf(F, "\"%s\",", str);
      for (rc = races; rc; rc = rc->next) {
        if (playerrace(rc)) {
          if (rc->bonus[sk])
            fprintf(F, "%d,", rc->bonus[sk]);
          else
            fputc(',', F);
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
  if (towlower(0xC4) != 0xE4) { /* &Auml; => &auml; */
    log_error(
      ("Umlaut conversion is not working properly. Wrong locale? LANG=%s\n",
        getenv("LANG")));
  }
}

extern void bind_eressea(struct lua_State *L);

int main(int argc, char **argv)
{
  static int write_csv = 0;
  int err, result = 0;

  setup_signal_handler();

  parse_config(inifile);

  err = parse_args(argc, argv, &result);
  if (err) {
    return result;
  }

  log_open("eressea.log");
  locale_init();

#ifdef CRTDBG
  init_crtdbg();
#endif

  err = eressea_init();
  if (err) {
    log_error(("initialization failed with code %d\n", err));
    return err;
  }
  register_races();
  register_curses();
  register_spells();
  bind_eressea((struct lua_State *)global.vm_state);

  if (write_csv) {
    dump_skills();
    dump_spells();
  }

  err = eressea_run(luafile, entry_point);
  if (err) {
    log_error(("server execution failed with code %d\n", err));
    return err;
  }
#ifdef MSPACES
  malloc_stats();
#endif

  eressea_done();
  log_close();
  if (global.inifile)
    iniparser_free(global.inifile);
  return 0;
}
