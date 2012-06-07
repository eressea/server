/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
                         Katja Zedel <katze@felidae.kn-bremen.de
                         Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <util/log.h>

#include <kernel/types.h>
#include <eressea.h>
#include <gmtool.h>
#include <kernel/config.h>
#include <kernel/save.h>
#include <bindings/bindings.h>
#include <iniparser.h>

#include <lua.h>

#include <assert.h>
#include <locale.h>
#include <wctype.h>

static const char *luafile = 0;
static const char *entry_point = NULL;
static const char *inifile = "eressea.ini";
static const char *logfile= "eressea.log";
static int memdebug = 0;

static void parse_config(const char *filename)
{
  dictionary *d = iniparser_new(filename);
  if (d) {
    load_inifile(d);
    log_debug("reading from configuration file %s\n", filename);

    memdebug = iniparser_getint(d, "eressea:memcheck", memdebug);
    entry_point = iniparser_getstring(d, "eressea:run", entry_point);
    luafile = iniparser_getstring(d, "eressea:load", luafile);

    /* only one value in the [editor] section */
    force_color = iniparser_getint(d, "editor:color", force_color);
  } else {
    log_error("could not open configuration file %s\n", filename);
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
    "--color          : force curses to use colors even when not detected\n", prog);
  return -1;
}

static int parse_args(int argc, char **argv, int *exitcode)
{
  int i;

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
      } else if (strcmp(argv[i] + 2, "help") == 0) {
        return usage(argv[0], NULL);
      } else {
        return usage(argv[0], argv[i]);
      }
    } else {
      switch (argv[i][1]) {
        case 'C':
          entry_point = NULL;
          break;
        case 'l':
          logfile = argv[i][2] ? argv[i]+2 : argv[++i];
          break;
        case 'e':
          entry_point = argv[i][2] ? argv[i]+2 : argv[++i];
          break;
        case 't':
          turn = atoi(argv[i][2] ? argv[i]+2 : argv[++i]);
          break;
        case 'q':
          verbosity = 0;
          break;
        case 'v':
          verbosity = atoi(argv[i][2] ? argv[i]+2 : argv[++i]);
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

  return 0;
}

void locale_init(void)
{
  setlocale(LC_CTYPE, "");
  setlocale(LC_NUMERIC, "C");
  assert(towlower(0xC4) == 0xE4);       /* &Auml; => &auml; */
}

int main(int argc, char **argv)
{
  int err, result = 0;
  lua_State * L;

  log_open(logfile);
  parse_config(inifile);

  err = parse_args(argc, argv, &result);
  if (err) {
    return result;
  }

  locale_init();

  L = lua_init();
  game_init();

  err = eressea_run(L, luafile, entry_point);
  if (err) {
    log_error("server execution failed with code %d\n", err);
    return err;
  }

  game_done();
  lua_done(L);
  log_close();
  return 0;
}
