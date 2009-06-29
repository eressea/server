#include <config.h>
#include <kernel/eressea.h>

#include "bindings.h"
#include "list.h"
#include "../gmtool.h"
#include "../gmtool_structs.h"

#include <kernel/region.h>

// lua includes
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

using namespace luabind;

region *
current_region(void)
{
  map_region * mr = cursor_region(&current_state->display, &current_state->cursor);
  return mr?mr->r:NULL;
}

static tag *
next_tag(int hash, const state * st)
{
  while (st && hash!=MAXTHASH) {
    tag * node = st->selected->tags[hash];
    while (node!=NULL) {
      region * r = findregion((short)node->coord.x, (short)node->coord.y);
      if (r) {
	return node;
      }
      node = node->nexthash;
    }
    ++hash;
  }
  return NULL;
}

class selectedregion {
public:
  static tag * next(tag * self) {
    tag * node = self->nexthash;
    while (node) {
      region * r = findregion((short)node->coord.x, (short)node->coord.y);
      if (r) return node;
      node = node->nexthash;
    }
    coordinate * c = &self->coord;
    unsigned int hash;
    int nx, ny;

    cnormalize(c, &nx, &ny);
    hash = ((nx << 12) ^ ny) & (MAXTHASH-1);

    return next_tag(hash+1, current_state);
  }

  static region * value(tag * node) {
    return findregion((short)node->coord.x, (short)node->coord.y);
  }
};


static eressea::list<region *, tag *, selectedregion>
selected_regions(void)
{
  return eressea::list<region *, tag *, selectedregion>(next_tag(0, current_state));
}

static void
gmtool_select_coordinate(int x, int y, bool select)
{
  int nx = x, ny = y;
  plane * pl = findplane(x, y);
  pnormalize(&nx, &ny, pl);
  select_coordinate(current_state->selected, nx, ny, select?1:0);
}

static void
gmtool_select_region(region& r, bool select)
{
  select_coordinate(current_state->selected, r.x, r.y, select?1:0);
}

static void gmtool_open(void) 
{
   state_open();
}

static void gmtool_close(void) 
{
   state_close(current_state);
}

void
bind_gmtool(lua_State * L)
{
  module(L, "gmtool")[
    def("open", &gmtool_open),
    def("close", &gmtool_close),
    def("editor", &run_mapper),
    def("get_selection", &selected_regions, return_stl_iterator),
    def("get_cursor", &current_region),
    def("highlight", &highlight_region),
    def("select", &gmtool_select_region),
    def("select_at", &gmtool_select_coordinate)
  ];
#ifdef LUABIND_NO_EXCEPTIONS
  luabind::set_error_callback(error_callback);
#endif
}
