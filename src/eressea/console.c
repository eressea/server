#include "console.h"

/* lua includes */
#include <lua50/lauxlib.h>
#include <lua50/lualib.h>

/* libc includes */
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*** Lua Console ***/
/*
** this macro defines a function to show the prompt and reads the
** next line for manual input
*/
#ifndef lua_readline
#define lua_readline(L,prompt) readline(L,prompt)

/* maximum length of an input line */
#ifndef MAXINPUT
#define MAXINPUT        512
#endif

static int 
readline(lua_State *l, const char *prompt) 
{
  static char buffer[MAXINPUT];
  if (prompt) {
    fputs(prompt, stdout);
    fflush(stdout);
  }
  if (fgets(buffer, sizeof(buffer), stdin) == NULL)
    return 0;  /* read fails */
  else {
    lua_pushstring(l, buffer);
    return 1;
  }
}
#endif

#ifndef PROMPT
#define PROMPT          "E> "
#endif

#ifndef PROMPT2
#define PROMPT2         "E>> "
#endif

static const char *
get_prompt(lua_State * L, int firstline) 
{
  const char *p = NULL;
  lua_pushstring(L, firstline ? "_PROMPT" : "_PROMPT2");
  lua_rawget(L, LUA_GLOBALSINDEX);
  p = lua_tostring(L, -1);
  if (p == NULL) p = (firstline ? PROMPT : PROMPT2);
  lua_pop(L, 1);  /* remove global */
  return p;
}

static int 
incomplete(lua_State * L, int status) 
{
  if (status!=LUA_ERRSYNTAX) return 0;
  if (strstr(lua_tostring(L, -1), "near `<eof>'") == NULL) return 0;

  lua_pop(L, 1);
  return 1;
}

static void 
l_message(const char *pname, const char *msg) 
{
  if (pname) fprintf(stderr, "%s: ", pname);
  fprintf(stderr, "%s\n", msg);
}

static int 
l_report(lua_State * L, int status) 
{
  const char *msg;
  if (status) {
    msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error with no message)";
    l_message(NULL, msg);
    lua_pop(L, 1);
  }
  return status;
}

/*
** this macro can be used by some `history' system to save lines
** read in manual input
*/
#ifndef lua_saveline
#define lua_saveline(L,line)    /* empty */
#endif

static int 
load_string(lua_State * L) 
{
  int status;
  lua_settop(L, 0);
  if (lua_readline(L, get_prompt(L, 1)) == 0)  /* no input? */
    return -1;
  if (lua_tostring(L, -1)[0] == '=') {  /* line starts with `=' ? */
    lua_pushfstring(L, "return %s", lua_tostring(L, -1)+1);/* `=' -> `return' */
    lua_remove(L, -2);  /* remove original line */
  }
  for (;;) {  /* repeat until gets a complete line */
    status = luaL_loadbuffer(L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin");
    if (!incomplete(L, status)) break;  /* cannot try to add lines? */
    if (lua_readline(L, get_prompt(L, 0)) == 0)  /* no more input? */
      return -1;
    lua_concat(L, lua_gettop(L));  /* join lines */
  }
  lua_saveline(L, lua_tostring(L, 1));
  lua_remove(L, 1);  /* remove line */
  return status;
}

static void 
lstop(lua_State *l, lua_Debug *ar) 
{
  (void)ar;  /* unused arg. */
  lua_sethook(l, NULL, 0, 0);
  luaL_error(l, "interrupted!");
}

static lua_State * global_state = NULL;
static void 
laction(int i) 
{
  signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
                      terminate process (default action) */
  assert(global_state!=NULL);
  lua_sethook(global_state, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

static int 
lcall(lua_State * L, int narg, int clear) 
{
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushliteral(L, "_TRACEBACK");
  lua_rawget(L, LUA_GLOBALSINDEX);  /* get traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  assert(global_state==NULL);
  global_state = L; /* baaaad hack */
  signal(SIGINT, laction);
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
  signal(SIGINT, SIG_DFL);
  assert(global_state==L);
  global_state=NULL;
  lua_remove(L, base);  /* remove traceback function */
  return status;
}

int
lua_console(lua_State * L)
{
  int status;
  while ((status = load_string(L)) != -1) {
    if (status == 0) status = lcall(L, 0, 0);
    l_report(L, status);
    if (status == 0 && lua_gettop(L) > 0) {  /* any result to print? */
      lua_getglobal(L, "print");
      lua_insert(L, 1);
      if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
        l_message(NULL, lua_pushfstring(L, "error calling `print' (%s)",
        lua_tostring(L, -1)));
    }
  }
  lua_settop(L, 0);  /* clear stack */
  fputs("\n", stdout);
  return 0;
}
