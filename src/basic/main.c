#include <platform.h>
#include <util/log.h>

#include <eressea.h>
#include <gmtool.h>
#include <kernel/config.h>
#include <iniparser/iniparser.h>

static const char * luafile = "init.lua";
static const char * entry_point = NULL;
static int memdebug = 0;

static void load_config(const char * filename)
{
  dictionary * d = iniparser_new(filename);
  if (d) {
    load_inifile(d);

    memdebug = iniparser_getint(d, "eressea:memcheck", memdebug);
    entry_point = iniparser_getstring(d, "eressea:run", entry_point);
    luafile = iniparser_getstring(d, "eressea:load", luafile);

    /* only one value in the [editor] section */
    force_color = iniparser_getint(d, "editor:color", force_color);
  }
  global.inifile = d;
}

int main(int argc, char ** argv)
{
  int err;

  log_open("eressea.log");
  load_config("eressea.ini");

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
