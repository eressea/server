#include <config.h>

#include "bind_gmtool.h"
#include "../gmtool.h"
#include "../gmtool_structs.h"

#include <kernel/region.h>

#include <lua.h>
#include <tolua.h>

static int
tolua_run_mapper(lua_State* L)
{
  run_mapper();
  return 0;
}

static int
tolua_highlight_region(lua_State* L)
{
  region * r = tolua_tousertype(L, 1, 0);
  int select = tolua_toboolean(L, 2, 0);
  highlight_region(r, select);
  return 0;
}

static int
tolua_current_region(lua_State* L)
{
  map_region * mr = cursor_region(&current_state->display, &current_state->cursor);
  tolua_pushusertype(L, mr?mr->r:NULL, "region");
  return 1;
}


static int
tolua_select_coordinate(lua_State* L)
{
  int x = (int)tolua_tonumber(L, 1, 0);
  int y = (int)tolua_tonumber(L, 2, 0);
  int select = tolua_toboolean(L, 3, 0);
  if (current_state) {
    select_coordinate(current_state->selected, x, y, select);
  }
  return 0;
}

static int
tolua_select_region(lua_State* L)
{
  region * r = tolua_tousertype(L, 1, 0);
  int select = tolua_toboolean(L, 2, 0);
  if (current_state) {
    select_coordinate(current_state->selected, r->x, r->y, select);
  }
  return 0;
}

typedef struct tag_iterator {
  selection * list;
  tag * node;
  region * r;
  int hash;
} tag_iterator;

void
tag_advance(tag_iterator * iter)
{
  while (iter->hash!=MAXTHASH) {
    if (iter->node) {
      iter->node = iter->node->nexthash;
    }
    while (!iter->node && iter->hash != MAXTHASH) {
      if (++iter->hash != MAXTHASH) {
        iter->node = iter->list->tags[iter->hash];
      }
    }
    if (iter->node) {
      iter->r = findregion((short)iter->node->coord.x, (short)iter->node->coord.y);
      if (iter->r) {
        break;
      }
    }
  }
}

void
tag_rewind(tag_iterator * iter)
{
  if (iter->list) {
    iter->r = NULL;
    iter->node = iter->list->tags[0];
    iter->hash = 0;
    if (iter->node) {
      iter->r = findregion((short)iter->node->coord.x, (short)iter->node->coord.y);
    }
    if (!iter->r) {
      tag_advance(iter);
    }
  } else {
    iter->node = 0;
    iter->hash = MAXTHASH;
  }
}

static int 
tolua_tags_next(lua_State *L)
{
  tag_iterator * iter = (tag_iterator *)lua_touserdata(L, lua_upvalueindex(1));
  if (iter->node) {
    tolua_pushusertype(L, (void*)iter->r, "region");
    tag_advance(iter);
    return 1;
  }
  else {
    return 0;  /* no more values to return */
  }
}

static int
tolua_selected_regions(lua_State* L)
{
  tag_iterator * iter = (tag_iterator*)lua_newuserdata(L, sizeof(tag_iterator));

  luaL_getmetatable(L, "tag_iterator");
  lua_setmetatable(L, -2);

  iter->list = current_state->selected;
  tag_rewind(iter);

  lua_pushcclosure(L, tolua_tags_next, 1);
  return 1;
}

static int
tolua_state_open(lua_State* L)
{
  unused(L);
  state_open();
  return 0;
}

static int
tolua_state_close(lua_State* L)
{
  unused(L);
  state_close(current_state);
  return 0;
}

void
tolua_gmtool_open(lua_State* L)
{
  /* register user types */
  tolua_usertype(L, "tag_iterator");

  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_module(L, "gmtool", 0);
    tolua_beginmodule(L, "gmtool");
    {
      tolua_function(L, "open", tolua_state_open);
      tolua_function(L, "close", tolua_state_close);

      tolua_function(L, "editor", tolua_run_mapper);
      tolua_function(L, "get_selection", tolua_selected_regions);
      tolua_function(L, "get_cursor", tolua_current_region);
      tolua_function(L, "highlight", tolua_highlight_region);
      tolua_function(L, "select", tolua_select_region);
      tolua_function(L, "select_at", tolua_select_coordinate);
    }
    tolua_endmodule(L);
  }
  tolua_endmodule(L);
}
