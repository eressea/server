#include <curses.h>

#include "bind_gmtool.h"
#include "gmtool.h"
#include "gmtool_structs.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <modules/autoseed.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/path.h>

#include <lua.h>
#include <lauxlib.h>
#include <tolua.h>

#include <string.h>

static int gmtool_run_mapper(lua_State * L)
{
    UNUSED_ARG(L);
    run_mapper();
    return 0;
}

static int gmtool_highlight_region(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    int select = tolua_toboolean(L, 2, 0);
    UNUSED_ARG(L);
    highlight_region(r, select);
    return 0;
}

static int gmtool_current_region(lua_State * L)
{
    map_region *mr =
        cursor_region(&current_state->display, &current_state->cursor);
    tolua_pushusertype(L, mr ? mr->r : NULL, "region");
    return 1;
}

static int gmtool_select_coordinate(lua_State * L)
{
    int nx = (int)tolua_tonumber(L, 1, 0);
    int ny = (int)tolua_tonumber(L, 2, 0);
    int select = tolua_toboolean(L, 3, 0);
    UNUSED_ARG(L);
    if (current_state) {
        select_coordinate(current_state->selected, nx, ny, select);
    }
    return 0;
}

static int gmtool_select_region(lua_State * L)
{
    region *r = tolua_tousertype(L, 1, 0);
    int select = tolua_toboolean(L, 2, 0);
    if (current_state && r) {
        select_coordinate(current_state->selected, r->x, r->y, select);
    }
    return 0;
}

typedef struct tag_iterator {
    selection *list;
    tag *node;
    region *r;
    int hash;
} tag_iterator;

void tag_advance(tag_iterator * iter)
{
    while (iter->hash != MAXTHASH) {
        if (iter->node) {
            iter->node = iter->node->nexthash;
        }
        while (!iter->node && iter->hash != MAXTHASH) {
            if (++iter->hash != MAXTHASH) {
                iter->node = iter->list->tags[iter->hash];
            }
        }
        if (iter->node) {
            iter->r = findregion(iter->node->coord.x, iter->node->coord.y);
            if (iter->r) {
                break;
            }
        }
    }
}

void tag_rewind(tag_iterator * iter)
{
    if (iter->list) {
        iter->r = NULL;
        iter->node = iter->list->tags[0];
        iter->hash = 0;
        if (iter->node) {
            iter->r = findregion(iter->node->coord.x, iter->node->coord.y);
        }
        if (!iter->r) {
            tag_advance(iter);
        }
    }
    else {
        iter->node = 0;
        iter->hash = MAXTHASH;
    }
}

static int gmtool_tags_next(lua_State * L)
{
    tag_iterator *iter = (tag_iterator *)lua_touserdata(L, lua_upvalueindex(1));
    if (iter->node) {
        tolua_pushusertype(L, (void *)iter->r, "region");
        tag_advance(iter);
        return 1;
    }
    else {
        return 0;                   /* no more values to return */
    }
}

static int gmtool_selected_regions(lua_State * L)
{
    tag_iterator *iter =
        (tag_iterator *)lua_newuserdata(L, sizeof(tag_iterator));

    luaL_getmetatable(L, "tag_iterator");
    lua_setmetatable(L, -2);

    iter->list = current_state->selected;
    tag_rewind(iter);

    lua_pushcclosure(L, gmtool_tags_next, 1);
    return 1;
}

static int gmtool_state_open(lua_State * L)
{
    UNUSED_ARG(L);
    state_open();
    return 0;
}

static int gmtool_state_close(lua_State * L)
{
    UNUSED_ARG(L);
    state_close(current_state);
    return 0;
}

static int gmtool_make_island(lua_State * L)
{
    int x = (int)tolua_tonumber(L, 1, 0);
    int y = (int)tolua_tonumber(L, 2, 0);
    int s = (int)tolua_tonumber(L, 3, 0);
    const char *path = tolua_tostring(L, 4, NULL);
    newfaction *new_players = NULL;
    if (path) {
        char sbuffer[512];
        path_join(basepath(), path, sbuffer, sizeof(sbuffer));
        new_players = read_newfactions(sbuffer);
    }

    s = build_island(x, y, s, &new_players, count_newfactions(new_players));
    lua_pushinteger(L, s);
    return 1;
}

static int paint_handle;
static struct lua_State *paint_state;

static void lua_paint_info(struct window *wnd, const struct state *st)
{
    struct lua_State *L = paint_state;
    int nx = st->cursor.x, ny = st->cursor.y;
    pnormalize(&nx, &ny, st->cursor.pl);
    lua_rawgeti(L, LUA_REGISTRYINDEX, paint_handle);
    lua_pushinteger(L, nx);
    lua_pushinteger(L, ny);
    if (lua_pcall(L, 2, 1, 0) != 0) {
        const char *error = lua_tostring(L, -1);
        log_error("paint function failed: %s\n", error);
        lua_pop(L, 1);
        tolua_error(L, "event handler call failed", NULL);
    }
    else {
        const char *result = lua_tostring(L, -1);
        WINDOW *win = wnd->handle;
        int size = getmaxx(win) - 2;
        int line = 0, maxline = getmaxy(win) - 2;
        const char *str = result;
        box(win, 0, 0);

        while (*str && line < maxline) {
            const char *end = strchr(str, '\n');
            if (!end)
                break;
            else {
                int bytes = (int)(end - str);
                if (bytes < size) bytes = size;
                mvwaddnstr(win, line++, 1, str, bytes);
                wclrtoeol(win);
                str = end + 1;
            }
        }
    }
}

static int gmtool_set_display(lua_State * L)
{
    int type = lua_type(L, 1);
    if (type == LUA_TFUNCTION) {
        lua_pushvalue(L, 1);
        paint_handle = luaL_ref(L, LUA_REGISTRYINDEX);
        paint_state = L;

        set_info_function(&lua_paint_info);
    }
    else {
        set_info_function(NULL);
    }
    return 0;
}

static int gmtool_make_block(lua_State * L)
{
    int x = (int)tolua_tonumber(L, 1, 0);
    int y = (int)tolua_tonumber(L, 2, 0);
    int r = (int)tolua_tonumber(L, 3, 6);
    const char *str = tolua_tostring(L, 4, "ocean");
    const struct terrain_type *ter = get_terrain(str);

    make_block(x, y, r, ter);
    return 0;
}

void tolua_gmtool_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, "tag_iterator");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_module(L, "gmtool", 0);
        tolua_beginmodule(L, "gmtool");
        {
            tolua_function(L, "open", &gmtool_state_open);
            tolua_function(L, "close", &gmtool_state_close);

            tolua_function(L, "editor", &gmtool_run_mapper);
            tolua_function(L, "get_selection", &gmtool_selected_regions);
            tolua_function(L, "get_cursor", &gmtool_current_region);
            tolua_function(L, "highlight", &gmtool_highlight_region);
            tolua_function(L, "select", &gmtool_select_region);
            tolua_function(L, "select_at", &gmtool_select_coordinate);
            tolua_function(L, "set_display", &gmtool_set_display);

            tolua_function(L, "make_block", &gmtool_make_block);
            tolua_function(L, "make_island", &gmtool_make_island);

            tolua_constant(L, "KEY_F1", KEY_F(1));
            tolua_constant(L, "KEY_F2", KEY_F(2));
            tolua_constant(L, "KEY_F3", KEY_F(3));
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
