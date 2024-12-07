#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <kernel/calendar.h>
#include <kernel/config.h>
#include <kernel/messages.h>
#include <kernel/version.h>

#include <util/language.h>
#include <util/log.h>
#include <util/stats.h>
#include <util/path.h>
#include <util/password.h>

#include "eressea.h"
#ifdef USE_CURSES
#include "gmtool.h"
#endif

#include "signals.h"
#include "bindings.h"

#include <iniparser.h>
#include <dictionary.h>

#include <lua.h>

#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

static const char *logfile = "eressea.log";
static const char *luafile = NULL;
static const char *inifile = "eressea.ini";
static int memdebug = 0;
static int verbosity = 2;

static void load_inifile(void)
{
    const char *str;

    str = config_get("game.base");
    if (str) {
        set_basepath(str);
    }
    str = config_get("game.report");
    if (str) {
        set_reportpath(str);
    }
    str = config_get("game.data");
    if (str) {
        set_datapath(str);
    }

    verbosity = config_get_int("game.verbose", 2);
    memdebug = config_get_int("game.memcheck", memdebug);
#ifdef USE_CURSES
    /* only one value in the [editor] section */
    force_color = config_get_int("editor.color", force_color);
#endif
}

static const char * valid_keys[] = {
    "game.id",
    "game.deadlog",
    "game.maxnmr",
    "game.name",
    "game.start",
    "game.seed",
    "game.locale",
    "game.verbose",
    "game.report",
    "game.memcheck",
    "game.email",
    "game.mailcmd",
    "game.era",
    "game.sender",
    "game.dbname",
    "game.dbswap",
    "game.dbbatch",
    "editor.color",
    "editor.codepage",
    "editor.population.",
    "lua.",
    NULL
};

static dictionary *parse_config(const char *filename)
{
    dictionary *d;
    const char *str, *cfgpath = config_get("config.path");

    if (cfgpath) {
        char path[PATH_MAX];
        path_join(cfgpath, filename, path, sizeof(path));
        log_debug("reading from configuration file %s\n", path);
        d = iniparser_load(path);
    }
    else {
        log_debug("reading from configuration file %s\n", filename);
        d = iniparser_load(filename);
    }
    if (d) {
        config_set_from(d, valid_keys);
        load_inifile();
    }
    str = config_get("game.locales");
    make_locales(str ? str : "de,en");
    return d;
}

static int usage(const char *prog, const char *arg)
{
    if (arg) {
        fprintf(stderr, "unknown argument: %s\n\n", arg);
    }
    fprintf(stderr, "Usage: %s [options]\n"
        "-t <turn>        : read this datafile, not the most current one\n"
        "-f <script.lua>  : execute a lua script\n"
        "-q               : be quite (same as -v 0)\n"
        "-v <level>       : verbosity level\n"
        "-C               : run in interactive mode\n"
        "--color          : force curses to use colors even when not detected\n", prog);
    return -1;
}

static int get_arg(int argc, char **argv, size_t len, int index, const char **result, const char *def) {
    if (argv[index][len]) {
        *result = argv[index] + len;
        return index;
    }
    if (index + 1 < argc) {
        *result = argv[index + 1];
        return index + 1;
    }
    *result = def;
    return index;
}

static int verbosity_to_flags(int value) {
    int flags = 0;
    switch (value) {
    case 0:
        flags = 0;
        break;
    case 1:
        flags = LOG_CPERROR;
        break;
    case 2:
        flags = LOG_CPERROR | LOG_CPWARNING;
        break;
    case 3:
        flags = LOG_CPERROR | LOG_CPWARNING | LOG_CPINFO;
        break;
    default:
        flags = LOG_CPERROR | LOG_CPWARNING | LOG_CPINFO | LOG_CPDEBUG;
        break;
    }
    return flags;
}

static int parse_args(int argc, char **argv)
{
    int i;
    int log_flags = 3;

    for (i = 1; i != argc; ++i) {
        char *argi = argv[i];
        if (argi[0] != '-') {
            luafile = argi;
        }
        else if (argi[1] == '-') {     /* long format */
            if (strcmp(argi + 2, "version") == 0) {
                printf("Eressea version %s, "
                    "Copyright (C) 2023 Enno Rehling et al.\n",
                    eressea_version());
                return 1;
#ifdef USE_CURSES          
            }
            else if (strcmp(argi + 2, "color") == 0) {
                /* force the editor to have colors */
                force_color = 1;
#endif          
            }
            else if (strcmp(argi + 2, "help") == 0) {
                return usage(argv[0], NULL);
            }
            else {
                return usage(argv[0], argi);
            }
        }
        else {
            const char *arg;
            switch (argi[1]) {
            case 'D':
                config_set("config.debug", "1");
                break;
            case 'i':
                i = get_arg(argc, argv, 2, i, &arg, 0);
                config_set("config.install", arg);
                break;
            case 'c':
                i = get_arg(argc, argv, 2, i, &arg, 0);
                config_set("config.path", arg);
                break;
            case 'r':
                i = get_arg(argc, argv, 2, i, &arg, 0);
                config_set("config.rules", arg);
                break;
            case 'f':
                i = get_arg(argc, argv, 2, i, &luafile, 0);
                break;
            case 'l':
                i = get_arg(argc, argv, 2, i, &arg, 0);
                log_flags = arg ? atoi(arg) : 0xff;
                break;
            case 't':
                i = get_arg(argc, argv, 2, i, &arg, 0);
                turn = atoi(arg);
                break;
            case 'w':
                i = get_arg(argc, argv, 2, i, &arg, 0);
                bcrypt_workfactor = arg ? atoi(arg) : 0xff;
                break;
            case 'q':
                verbosity = 0;
                break;
            case 'v':
                i = get_arg(argc, argv, 2, i, &arg, 0);
                verbosity = arg ? atoi(arg) : 0xff;
                break;
            case 'h':
                usage(argv[0], NULL);
                return 1;
            default:
                usage(argv[0], argi);
                return 1;
            }
        }
    }

    /* open logfile on disk: */
    log_flags = verbosity_to_flags(log_flags);
    log_open(logfile, log_flags);

    /* also log to stderr: */
    log_flags = verbosity_to_flags(verbosity);
    if (log_flags) {
        log_to_file(log_flags | LOG_FLUSH | LOG_BRIEF, stderr);
    }
    return 0;
}

void locale_init(void)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_NUMERIC, "C");
    if (towlower(0xC4) != 0xE4) { /* &Auml; => &auml; */
        log_error("Umlaut conversion is not working properly. Wrong locale? LANG=%s\n",
            getenv("LANG"));
    }
}

int main(int argc, char **argv)
{
    int err = 0;
    lua_State *L;
    dictionary *d = NULL;
    setup_signal_handler();
    message_handle_missing(MESSAGE_MISSING_REPLACE);
    /* parse arguments again, to override ini file */
    err = parse_args(argc, argv);
    if (err != 0) {
        return (err > 0) ? 0 : err;
    }
    d = parse_config(inifile);
    if (!d) {
        log_error("could not open ini configuration %s\n", inifile);
    }

    locale_init();

    L = lua_init(d);
    game_init();
    bind_monsters(L);
    err = eressea_run(L, luafile);
    if (err) {
        log_error("script %s failed with code %d\n", luafile, err);
        return err;
    }
    game_done();
    lua_done(L);
    log_close();
    stats_write(stdout, "");
    stats_close();
    if (d) {
        iniparser_freedict(d);
    }
    return 0;
}
