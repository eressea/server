#include <config.h>

#include "bind_gmtool.h"
#include "../gmtool.h"
#include "../gmtool_structs.h"

#include <kernel/region.h>

#include <lua.h>
#include <tolua++.h>

static int
tolua_run_mapper(lua_State* tolua_S)
{
  run_mapper();
  return 0;
}

static int
tolua_highlight_region(lua_State* tolua_S)
{
  region * r = tolua_tousertype(tolua_S, 1, 0);
  int select = tolua_toboolean(tolua_S, 2, 0);
  highlight_region(r, select);
  return 0;
}

static int
tolua_current_region(lua_State* tolua_S)
{
  map_region * mr = cursor_region(&current_state->display, &current_state->cursor);
  tolua_pushusertype(tolua_S, mr?mr->r:NULL, "region");
  return 1;
}


static int
tolua_select_coordinate(lua_State* tolua_S)
{
  int x = (int)tolua_tonumber(tolua_S, 1, 0);
  int y = (int)tolua_tonumber(tolua_S, 2, 0);
  int select = tolua_toboolean(tolua_S, 3, 0);
  if (current_state) {
    select_coordinate(current_state->selected, x, y, select);
  }
  return 0;
}

static int
tolua_select_region(lua_State* tolua_S)
{
  region * r = tolua_tousertype(tolua_S, 1, 0);
  int select = tolua_toboolean(tolua_S, 2, 0);
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
tolua_tags_next(lua_State *tolua_S)
{
  tag_iterator * iter = (tag_iterator *)lua_touserdata(tolua_S, lua_upvalueindex(1));
  if (iter->node) {
    tolua_pushusertype(tolua_S, (void*)iter->r, "region");
    tag_advance(iter);
    return 1;
  }
  else {
    return 0;  /* no more values to return */
  }
}

static int
tolua_selected_regions(lua_State* tolua_S)
{
  tag_iterator * iter = (tag_iterator*)lua_newuserdata(tolua_S, sizeof(tag_iterator));

  luaL_getmetatable(tolua_S, "tag_iterator");
  lua_setmetatable(tolua_S, -2);

  iter->list = current_state->selected;
  tag_rewind(iter);

  lua_pushcclosure(tolua_S, tolua_tags_next, 1);
  return 1;
}

static int
tolua_state_open(lua_State* tolua_S)
{
  unused(tolua_S);
  state_open();
  return 0;
}

static int
tolua_state_close(lua_State* tolua_S)
{
  unused(tolua_S);
  state_close(current_state);
  return 0;
}

void
tolua_gmtool_open(lua_State* tolua_S)
{
  /* register user types */
  tolua_usertype(tolua_S, "tag_iterator");

  tolua_module(tolua_S, NULL, 0);
  tolua_beginmodule(tolua_S, NULL);
  {
    tolua_module(tolua_S, "gmtool", 0);
    tolua_beginmodule(tolua_S, "gmtool");
    {
      tolua_function(tolua_S, "open", tolua_state_open);
      tolua_function(tolua_S, "close", tolua_state_close);

      tolua_function(tolua_S, "editor", tolua_run_mapper);
      tolua_function(tolua_S, "get_selection", tolua_selected_regions);
      tolua_function(tolua_S, "get_cursor", tolua_current_region);
      tolua_function(tolua_S, "highlight", tolua_highlight_region);
      tolua_function(tolua_S, "select", tolua_select_region);
      tolua_function(tolua_S, "select_at", tolua_select_coordinate);
    }
    tolua_endmodule(tolua_S);
  }
  tolua_endmodule(tolua_S);
}
