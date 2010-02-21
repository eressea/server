#include <platform.h>
#include <util/log.h>
#include <kernel/eressea.h>

int eressea_init(void)
{
  global.vm_state = lua_init();
  kernel_init();
  game_init();

  return 0;
}

void eressea_done(void)
{
  game_done();
  kernel_done();
  lua_done((lua_State *)global.vm_state);
  log_close();
}

static int
log_lua_error(lua_State * L)
{
  static int s_abort_on_errors = 0;
  const char* error = lua_tostring(L, -1);

  log_error(("A LUA error occurred: %s\n", error));
  lua_pop(L, 1);

  if (s_abort_on_errors) {
    abort();
  }
  return 1;
}

int eressea_run(const char * luafile, const char * entry_point)
{
  lua_State * L = (lua_State *)global.vm_state;
  /* run the main script */
  if (luafile) {
    char buf[MAX_PATH];
    strcpy(buf, luafile);
    lua_getglobal(L, "dofile");
    lua_pushstring(L, buf);
    if (lua_pcall(L, 1, 0, 0) != 0) {
      log_lua_error(L);
    }
  }
  if (entry_point) {
    lua_getglobal(L, entry_point);
    if (lua_pcall(L, 0, 1, 0) != 0) {
      log_lua_error(L);
    }
  } else {
    lua_console(L);
  }
  return 0;
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

int main(int argc, char ** argv)
{
  int err;

  load_inifile("eressea.ini");

  err = eressea_init();
  if (err) {
    log_error(("initialization failed with code %d\n", err));
    return err;
  }

  err = eressea_run(luafile);
  if (err) {
    log_error(("server execution failed with code %d\n", err));
    return err;
  }

  eressea_done();
  return 0;
}
