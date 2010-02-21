#include <platform.h>
#include <util/log.h>

#include <eressea.h>
#include <kernel/config.h>
#include <iniparser/iniparser.h>

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

int main(int argc, char ** argv)
{
  int err;

  load_inifile("eressea.ini");

  err = eressea_init();
  if (err) {
    log_error(("initialization failed with code %d\n", err));
    return err;
  }

  err = eressea_run(luafile, entry_point);
  if (err) {
    log_error(("server execution failed with code %d\n", err));
    return err;
  }

  eressea_done();
  return 0;
}
