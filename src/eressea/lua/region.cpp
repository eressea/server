#include <config.h>
#include <kernel/eressea.h>
#include "list.h"
#include "objects.h"

// kernel includes
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/plane.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/log.h>

#include <attributes/key.h>

// lua includes
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#include <luabind/operator.hpp>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

#include <ostream>
using namespace luabind;

static eressea::list<region *>
get_regions(void) {
  return eressea::list<region *>(regions);
}

static eressea::list<unit *>
region_units(const region * r) {
  return eressea::list<unit *>(r->units);
}

static eressea::list<building *>
region_buildings(const region * r) {
  return eressea::list<building *>(r->buildings);
}

static eressea::list<ship *>
region_ships(const region * r) {
  return eressea::list<ship *>(r->ships);
}

static const char *
region_getterrain(const region * r) {
  return (const char *)r->terrain->_name;
}

static void
lua_region_setowner(region * r, faction * f) {
  region_setowner(r, f);
}

static faction *
lua_region_getowner(const region * r) {
  return region_owner(r);
}

static void
region_setherbtype(region * r, const char * str) {
  const struct resource_type * rtype = rt_find(str);
  if (rtype!=NULL && rtype->itype!=NULL) {
    rsetherbtype(r, rtype->itype);
  }
}

static const char *
region_getherbtype(const region * r) {
  const struct item_type * itype = rherbtype(r);
  if (itype==NULL) return NULL;
  return itype->rtype->_name[0];
}

static void
region_setinfo(region * r, const char * info)
{
  free(r->display);
  r->display = strdup(info);
}

static const char *
region_getinfo(const region * r) {
  return (const char *)r->display;
}

static int
region_plane(const region * r)
{
  if (r->planep==NULL) return 0;
  return r->planep->id;
}

static void
region_addnotice(region * r, const char * str)
{
  addmessage(r, NULL, str, MSG_MESSAGE, ML_IMPORTANT);
}

static std::ostream&
operator<<(std::ostream& stream, const region& r)
{
  stream << regionname(&r, NULL) << ", " << (const char *)r.terrain->_name;
  return stream;
}

static bool
operator==(const region& a, const region& b)
{
  return a.x==b.x && a.y==b.y;
}

static bool
region_getflag(const region * r, int bit)
{
  if (r->flags & (1<<bit)) return true;
  return false;
}

static void
region_setflag(region * r, int bit, bool set)
{
  if (set) r->flags |= (1<<bit);
  else r->flags &= ~(1<<bit);
}

static int
region_get_resource(const region * r, const char * type)
{
  const resource_type * rtype = rt_find(type);
  if (rtype!=NULL) {
    return region_getresource(r, rtype);
  } else {
    if (strcmp(type, "seed")==0) return rtrees(r, 0);
    if (strcmp(type, "sapling")==0) return rtrees(r, 1);
    if (strcmp(type, "tree")==0) return rtrees(r, 2);
    if (strcmp(type, "grave")==0) return deathcount(r);
    if (strcmp(type, "chaos")==0) return chaoscount(r);
  }
  return 0;
}

static void
region_set_resource(region * r, const char * type, int value)
{
  const resource_type * rtype = rt_find(type);
  if (rtype!=NULL) {
    region_setresource(r, rtype, value);
  } else {
    if (strcmp(type, "seed")==0) {
      rsettrees(r, 0, value);
    } else if (strcmp(type, "sapling")==0) {
      rsettrees(r, 1, value);
    } else if (strcmp(type, "tree")==0) {
      rsettrees(r, 2, value);
    } else if (strcmp(type, "grave")==0) {
      int fallen = value-deathcount(r);
      deathcounts(r, fallen);
    } else if (strcmp(type, "chaos")==0) {
      int fallen = value-chaoscount(r);
      chaoscounts(r, fallen);
    }
  }
}

static void
region_setroad(region * r, int dir, lua_Number size)
{
  if (r->terrain->max_road>0) {
    rsetroad(r, (direction_t)dir, (short)(r->terrain->max_road * size));
  }
}

static lua_Number
region_getroad(region * r, int dir)
{
  lua_Number result = rroad(r, (direction_t)dir);
  if (r->terrain->max_road<=0 || result<=0) return 0;
  return r->terrain->max_road / result;
}

static region *
region_terraform(short x, short y, const char * tname)
{
  const terrain_type * terrain = get_terrain(tname);
  region * r = findregion(x, y);
  if (terrain==NULL) {
    if (r!=NULL) {
      if (r->units!=NULL) {
        // TODO: error message
        return r;
      }
      // TODO: region löschen
      terraform_region(r, NULL);
    }
    return NULL;
  }
  if (r==NULL) r = new_region(x, y, 0);
  terraform_region(r, terrain);
  return r;
}

static region *
region_next(const region * r, int dir)
{
  if (dir<0 || dir >=MAXDIRECTIONS) return NULL;
  return r_connect(r, (direction_t)dir);
}

static void
region_adddirection(region * r, region * rt, const char * name, const char * info)
{
  create_special_direction(r, rt, -1, info, name);
  spec_direction * sd = special_direction(r, rt);
  sd->active = 1;
}

static void
region_remove(region * r)
{
  remove_region(&regions, r);
}

static void
plane_remove(int plane_id)
{
  region ** rp = &regions;
  while (*rp) {
    region * r = *rp;
    if (r->planep && r->planep->id==plane_id) {
      remove_region(rp, r);
    } else {
      rp = &r->next;
    }
  }
}

void
region_move(region * r, short x, short y)
{
  if (findregion(x,y)) {
    log_error(("Bei %d, %d gibt es schon eine Region.\n", x, y));
    return;
  }
#ifdef FAST_CONNECT
  direction_t dir;
  for (dir=0;dir!=MAXDIRECTIONS;++dir) {
    region * rn = r->connect[dir];
    if (rn!=NULL) {
      direction_t reldir = reldirection(rn, r);
      rn->connect[reldir] = NULL;
    }
    rn = findregion(x+delta_x[dir], y+delta_y[dir]);
    if (rn!=NULL) {
      direction_t reldir = (direction_t)((dir + 3) % MAXDIRECTIONS);
      rn->connect[reldir] = r;
    }
    r->connect[dir] = rn;
  }
#endif
  runhash(r);
  r->x = x;
  r->y = y;
  rhash(r);
}

static eressea::list<std::string, item *, eressea::bind_items>
region_items(const region * r) {
  if (r->land) {
    return eressea::list<std::string, item *, eressea::bind_items>(r->land->items);
  } else {
    return eressea::list<std::string, item *, eressea::bind_items>(NULL);
  }
}

static int
region_additem(region * r, const char * iname, int number)
{
  if (iname!=NULL) {
    const item_type * itype = it_find(iname);
    if (itype!=NULL && r->land) {
      item * i = i_change(&r->land->items, itype, number);
      return i?i->number:0;
    } // if (itype!=NULL)
  }
  return -1;
}

static bool
region_getkey(region * r, const char * name)
{
  int flag = atoi36(name);
  attrib * a = find_key(r->attribs, flag);
  return (a!=NULL);
}

static void
region_setkey(region * r, const char * name, bool value)
{
  int flag = atoi36(name);
  attrib * a = find_key(r->attribs, flag);
  if (a==NULL && value) {
    add_key(&r->attribs, flag);
  } else if (a!=NULL && !value) {
    a_remove(&r->attribs, a);
  }
}

static int
plane_getbyname(const char * str)
{
  plane * pl = getplanebyname(str);
  if (pl) return pl->id;
  return 0;
}

void
bind_region(lua_State * L)
{
  module(L)[
    def("regions", &get_regions, return_stl_iterator),
    def("get_region", &findregion),
    def("get_region_by_id", &findregionbyid),
    def("terraform", &region_terraform),
    def("distance", &distance),
    def("remove_region", &region_remove),
    def("remove_plane", &plane_remove),
    def("getplanebyname", &plane_getbyname),

    class_<struct region>("region")
    .def(tostring(const_self))
    .def(self == region())
    .property("name", &region_getname, &region_setname)
    .property("info", &region_getinfo, &region_setinfo)
    .property("owner", &lua_region_getowner, &lua_region_setowner)
    .property("herbtype", &region_getherbtype, &region_setherbtype)
    .property("terrain", &region_getterrain)
    .def("add_notice", &region_addnotice)
    .def("add_direction", &region_adddirection)

    .def("get_key", &region_getkey)
    .def("set_key", &region_setkey)

    .def("get_flag", &region_getflag)
    .def("set_flag", &region_setflag)

    .def("move", &region_move)

    .def("get_road", &region_getroad)
    .def("set_road", &region_setroad)

    .def("next", &region_next)
    .def("get_resource", &region_get_resource)
    .def("set_resource", &region_set_resource)
    .def_readonly("x", &region::x)
    .def_readonly("y", &region::y)
    .def_readonly("id", &region::uid)
    .def_readwrite("age", &region::age)
    .def("add_item", &region_additem)
    .property("items", &region_items, return_stl_iterator)
    .property("plane_id", &region_plane)
    .property("units", &region_units, return_stl_iterator)
    .property("buildings", &region_buildings, return_stl_iterator)
    .property("ships", &region_ships, return_stl_iterator)
    .property("objects", &eressea::get_objects<region>)
    .scope [
      def("create", &region_terraform)
    ]
  ];
}
