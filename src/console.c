#ifdef _MSC_VER
#include <platform.h>
#endif
#include "console.h"

/* lua includes */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

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
/* maximum length of an input line */
#ifndef LUA_MAXINPUT
#define LUA_MAXINPUT        512
#endif

#if LUA_VERSION_NUM >= 502
#define lua_strlen(L, idx) lua_rawlen(L, idx)
#endif

#if defined(LUA_USE_READLINE)
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define default_readline(L,b,p)	((void)L, ((b)=readline(p)) != NULL)
#define lua_saveline(L,idx) \
  if (lua_strlen(L,idx) > 0)  /* non-empty line? */ \
  add_history(lua_tostring(L, idx));    /* add it to history */
#define lua_freeline(L,b)	((void)L, free(b))
#else
#define default_readline(L,b,p)	\
  ((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */ \
  fgets(b, LUA_MAXINPUT, stdin) != NULL)        /* get line */
#define lua_saveline(L,idx)	{ (void)L; (void)idx; }
#define lua_freeline(L,b)	{ (void)L; (void)b; }
#endif

static int(*my_readline) (lua_State * l, char *, size_t, const char *prompt) =
NULL;
static int lua_readline(lua_State * l, char *b, const char *prompt)
{
    if (my_readline) {
        return my_readline(l, b, LUA_MAXINPUT, prompt);
    }
    else {
        return default_readline(l, b, prompt);
    }
}

void set_readline(readline foo)
{
    my_readline = foo;
}

#ifndef PROMPT
#define PROMPT          "E> "
#endif

#ifndef PROMPT2
#define PROMPT2         ".. "
#endif

/* BAD hack, all this action stuff. */
#define STATESTACK_MAX 16
static lua_State *state_stack[STATESTACK_MAX];
int state_stack_top = -1;

static const char *progname = "eressea";

static void lstop(lua_State * l, lua_Debug * ar)
{
    (void)ar;                     /* unused arg. */
    lua_sethook(l, NULL, 0, 0);
    luaL_error(l, "interrupted!");
}

static void laction(int i)
{
    signal(i, SIG_DFL);           /* if another SIGINT happens before lstop,
                                     terminate process (default action) */
    assert(state_stack_top >= 0 && state_stack_top < STATESTACK_MAX);
    lua_sethook(state_stack[state_stack_top], lstop,
        LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

static void l_message(const char *pname, const char *msg)
{
    if (pname)
        fprintf(stderr, "%s: ", pname);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
}

static int report(lua_State * L, int status)
{
    if (status && !lua_isnil(L, -1)) {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL)
            msg = "(error object is not a string)";
        l_message(progname, msg);
        lua_pop(L, 1);
    }
    return status;
}

static int traceback(lua_State * L)
{
    lua_getglobal(L, "debug");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 1;
    }
    lua_pushvalue(L, 1);          /* pass error message */
    lua_pushinteger(L, 2);        /* skip this function and traceback */
    lua_call(L, 2, 1);            /* call debug.traceback */
    return 1;
}

static int docall(lua_State * L, int narg, int clear)
{
    int status, pop_state = state_stack_top;
    int base = lua_gettop(L) - narg;      /* function index */
    lua_pushcfunction(L, traceback);      /* push traceback function */
    lua_insert(L, base);          /* put it under chunk and args */
    if (state_stack_top < 0 || L != state_stack[state_stack_top]) {
        assert(state_stack_top + 1 < STATESTACK_MAX);
        state_stack[++state_stack_top] = L;
    }
    signal(SIGINT, laction);
    status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
    signal(SIGINT, SIG_DFL);
    state_stack_top = pop_state;
    lua_remove(L, base);          /* remove traceback function */
    /* force a complete garbage collection in case of errors */
    if (status != 0)
        lua_gc(L, LUA_GCCOLLECT, 0);
    return status;
}

static const char *get_prompt(lua_State * L, int firstline)
{
    const char *p = NULL;
    lua_getglobal(L, firstline ? "_PROMPT" : "_PROMPT2");
    p = lua_tostring(L, -1);
    if (p == NULL)
        p = (firstline ? PROMPT : PROMPT2);
    lua_pop(L, 1);                /* remove global */
    return p;
}

static int incomplete(lua_State * L, int status)
{
    if (status != LUA_ERRSYNTAX)
        return 0;
    if (strstr(lua_tostring(L, -1), "near `<eof>'") == NULL)
        return 0;

    lua_pop(L, 1);
    return 1;
}

static int pushline(lua_State * L, int firstline)
{
    char buffer[LUA_MAXINPUT];
    char *b = buffer;
    size_t l;
    const char *prmt = get_prompt(L, firstline);
    if (lua_readline(L, b, prmt) == 0)
        return 0;                   /* no input */
    l = strlen(b);
    if (l > 0 && b[l - 1] == '\n')        /* line ends with newline? */
        b[l - 1] = '\0';            /* remove it */
    if (firstline && b[0] == '=') /* first line starts with `=' ? */
        lua_pushfstring(L, "return %s", b + 1);     /* change it to `return' */
    else
        lua_pushstring(L, b);
    lua_freeline(L, b);
    return 1;
}

static int loadline(lua_State * L)
{
    int status;
    lua_settop(L, 0);
    if (!pushline(L, 1))
        return -1;                  /* no input */
    for (;;) {                    /* repeat until gets a complete line */
        status = luaL_loadbuffer(L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin");
        if (!incomplete(L, status))
            break;                    /* cannot try to add lines? */
        if (!pushline(L, 0))        /* no more input? */
            return -1;
        lua_pushliteral(L, "\n");   /* add a new line... */
        lua_insert(L, -2);          /* ...between the two lines */
        lua_concat(L, 3);           /* join them */
    }
    lua_saveline(L, 1);
    lua_remove(L, 1);             /* remove line */
    return status;
}

static void dotty(lua_State * L)
{
    int status;
    const char *oldprogname = progname;
    progname = NULL;
    while ((status = loadline(L)) != -1) {
        if (status == 0)
            status = docall(L, 0, 0);
        report(L, status);
        if (status == 0 && lua_gettop(L) > 0) {     /* any result to print? */
            lua_getglobal(L, "print");
            lua_insert(L, 1);
            if (lua_pcall(L, lua_gettop(L) - 1, 0, 0) != 0)
                l_message(progname, lua_pushfstring(L,
                "error calling " LUA_QL("print") " (%s)", lua_tostring(L, -1)));
        }
    }
    lua_settop(L, 0);             /* clear stack */
    fputs("\n", stdout);
    fflush(stdout);
    progname = oldprogname;
}

int lua_console(lua_State * L)
{
    dotty(L);
    return 0;
}

int lua_do(lua_State * L)
{
    int status = loadline(L);
    if (status != -1) {
        if (status == 0) {
            status = docall(L, 0, 0);
        }
        report(L, status);
        if (status == 0 && lua_gettop(L) > 0) {     /* any result to print? */
            lua_getglobal(L, "print");
            lua_insert(L, 1);
            if (lua_pcall(L, lua_gettop(L) - 1, 0, 0) != 0) {
                l_message(NULL, lua_pushfstring(L, "error calling `print' (%s)",
                    lua_tostring(L, -1)));
            }
        }
    }
    lua_settop(L, 0);             /* clear stack */
    return 0;
}
