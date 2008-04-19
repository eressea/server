#include <config.h>
#include <kernel/eressea.h>

#include "bindings.h"
#include "list.h"
#include "../gmtool.h"
#include "../gmtool_structs.h"

#include <kernel/region.h>

// lua includes
#pragma warning (push)
#pragma warning (disable: 4127)
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#pragma warning (pop)

using namespace luabind;

region *
current_region(void)
{
  map_region * mr = cursor_region(&current_state->display, &current_state->cursor);
  return mr->r;
}

static tag *
next_tag(int hash, const state * st)
{
  while (hash!=MAXTHASH) {
    tag * t = st->selected->tags[hash];
    if (t!=NULL) return t;
    ++hash;
  }
  return NULL;
}

class selectedregion {
public:
  static tag * next(tag * node) {
    if (node->nexthash) {
      return node->nexthash;
    }
    coordinate * c = &node->coord;
    unsigned int key = ((c->x << 12) ^ c->y);
    unsigned int hash = key & (MAXTHASH-1);

    return next_tag(++hash, current_state);
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
gmtool_select_coordinate(int x, int y)
{
  select_coordinate(current_state->selected, x, y);
}

static void
gmtool_select_region(region& r)
{
  select_coordinate(current_state->selected, r.x, r.y);
}

void
bind_gmtool(lua_State * L)
{
  module(L, "gmtool")[
    def("editor", &run_mapper),
    def("selection", &selected_regions, return_stl_iterator),
    def("cursor", &current_region),
    def("highlight", &highlight_region),
    def("select", &gmtool_select_region),
    def("select_at", &gmtool_select_coordinate)
  ];
#ifdef LUABIND_NO_EXCEPTIONS
  luabind::set_error_callback(error_callback);
#endif
}
