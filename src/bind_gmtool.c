#include <platform.h>
#include <curses.h>

#include "bind_gmtool.h"
#include "gmtool.h"
#include "gmtool_structs.h"

#include <kernel/region.h>
#include <kernel/terrain.h>
#include <modules/autoseed.h>
#include <util/log.h>

#include <tolua.h>

#include <string.h>

static int tolua_run_mapper(lua_State * L)
{
    run_mapper();
    return 0;
}

static int tolua_highlight_region(lua_State * L)
{
    region *r = (region *)tolua_tousertype(L, 1, 0);
    int select = tolua_toboolean(L, 2, 0);
    highlight_region(r, select);
    return 0;
}

static int tolua_current_region(lua_State * L)
{
    map_region *mr =
        cursor_region(&current_state->display, &current_state->cursor);
    tolua_pushusertype(L, mr ? mr->r : NULL, TOLUA_CAST "region");
    return 1;
}

static int tolua_select_coordinate(lua_State * L)
{
    int nx = (int)tolua_tonumber(L, 1, 0);
    int ny = (int)tolua_tonumber(L, 2, 0);
    int select = tolua_toboolean(L, 3, 0);
    if (current_state) {
        select_coordinate(current_state->selected, nx, ny, select);
    }
    return 0;
}

static int tolua_select_region(lua_State * L)
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

static int tolua_tags_next(lua_State * L)
{
    tag_iterator *iter = (tag_iterator *)lua_touserdata(L, lua_upvalueindex(1));
    if (iter->node) {
        tolua_pushusertype(L, (void *)iter->r, TOLUA_CAST "region");
        tag_advance(iter);
        return 1;
    }
    else {
        return 0;                   /* no more values to return */
    }
}

static int tolua_selected_regions(lua_State * L)
{
    tag_iterator *iter =
        (tag_iterator *)lua_newuserdata(L, sizeof(tag_iterator));

    luaL_getmetatable(L, "tag_iterator");
    lua_setmetatable(L, -2);

    iter->list = current_state->selected;
    tag_rewind(iter);

    lua_pushcclosure(L, tolua_tags_next, 1);
    return 1;
}

static int tolua_state_open(lua_State * L)
{
    unused_arg(L);
    state_open();
    return 0;
}

static int tolua_state_close(lua_State * L)
{
    unused_arg(L);
    state_close(current_state);
    return 0;
}

static int tolua_make_island(lua_State * L)
{
    int x = (int)tolua_tonumber(L, 1, 0);
    int y = (int)tolua_tonumber(L, 2, 0);
    int s = (int)tolua_tonumber(L, 3, 0);
    int n = (int)tolua_tonumber(L, 4, s / 3);

    n = build_island_e3(NULL, x, y, n, s);
    lua_pushinteger(L, n);
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
        tolua_error(L, TOLUA_CAST "event handler call failed", NULL);
    }
    else {
        const char *result = lua_tostring(L, -1);
        WINDOW *win = wnd->handle;
        int size = getmaxx(win) - 2;
        int line = 0, maxline = getmaxy(win) - 2;
        const char *str = result;
        wxborder(win);

        while (*str && line < maxline) {
            const char *end = strchr(str, '\n');
            if (!end)
                break;
            else {
                size_t len = end - str;
                int bytes = _min((int)len, size);
                mvwaddnstr(win, line++, 1, str, bytes);
                wclrtoeol(win);
                str = end + 1;
            }
        }
    }
}

static int tolua_set_display(lua_State * L)
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

static int tolua_make_block(lua_State * L)
{
    int x = (int)tolua_tonumber(L, 1, 0);
    int y = (int)tolua_tonumber(L, 2, 0);
    int r = (int)tolua_tonumber(L, 3, 6);
    const char *str = tolua_tostring(L, 4, TOLUA_CAST "ocean");
    const struct terrain_type *ter = get_terrain(str);

    make_block(x, y, r, ter);
    return 0;
}

void tolua_gmtool_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "tag_iterator");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_module(L, TOLUA_CAST "gmtool", 0);
        tolua_beginmodule(L, TOLUA_CAST "gmtool");
        {
            tolua_function(L, TOLUA_CAST "open", &tolua_state_open);
            tolua_function(L, TOLUA_CAST "close", &tolua_state_close);

            tolua_function(L, TOLUA_CAST "editor", &tolua_run_mapper);
            tolua_function(L, TOLUA_CAST "get_selection", &tolua_selected_regions);
            tolua_function(L, TOLUA_CAST "get_cursor", &tolua_current_region);
            tolua_function(L, TOLUA_CAST "highlight", &tolua_highlight_region);
            tolua_function(L, TOLUA_CAST "select", &tolua_select_region);
            tolua_function(L, TOLUA_CAST "select_at", &tolua_select_coordinate);
            tolua_function(L, TOLUA_CAST "set_display", &tolua_set_display);

            tolua_function(L, TOLUA_CAST "make_block", &tolua_make_block);
            tolua_function(L, TOLUA_CAST "make_island", &tolua_make_island);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}
