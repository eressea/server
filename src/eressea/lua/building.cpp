#include <config.h>
#include <cstring>
#include <eressea.h>
#include "list.h"

// kernel includes
#include <building.h>
#include <region.h>
#include <unit.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

// util includes
#include <util/base36.h>

using namespace luabind;

static building *
add_building(region * r, const char * name)
{
  const building_type * btype = bt_find(name);
  if (btype==NULL) return NULL;
  return new_building(btype, r, NULL);
}

typedef struct lcbuilding_data {
  building * b;
  char * fname;
} lcbuilding_data;

static void 
lc_init(struct attrib *a)
{
  a->data.v = calloc(1, sizeof(lcbuilding_data));
}

static void 
lc_done(struct attrib *a)
{
  lcbuilding_data * data = (lcbuilding_data*)a->data.v;
  if (data->fname) free(data->fname);
  free(data);
}

static int
lc_age(struct attrib * a)
{
  lua_State * L = (lua_State *)global.vm_state;
  lcbuilding_data * data = (lcbuilding_data*)a->data.v;
  const char * fname = data->fname;
  building * b = data->b;

  assert(b!=NULL);
  if (fname==NULL) return -1;

  luabind::object globals = luabind::get_globals(L);
  if (globals.at(fname).type()!=LUA_TFUNCTION) return -1;
  
  return luabind::call_function<int>(L, fname, *b);
}

static void 
lc_write(const struct attrib * a, FILE* F)
{
  lcbuilding_data * data = (lcbuilding_data*)a->data.v;
  const char * fname = data->fname;
  building * b = data->b;

  write_building_reference(b, F);
  fwritestr(F, fname);
}

static int
lc_read(struct attrib * a, FILE* F)
{
  char lbuf[256];
  lcbuilding_data * data = (lcbuilding_data*)a->data.v;

  read_building_reference(&data->b, F);
  freadstr(F, lbuf, sizeof(lbuf));
  data->fname = strdup(lbuf);
  return AT_READ_OK;
}

attrib_type at_luacall_building = {
  "lcbuilding", 
  lc_init, 
  lc_done, 
  lc_age, 
  lc_write, 
  lc_read,
  ATF_UNIQUE
};

static int
building_addaction(building& b, const char * fname)
{
  attrib * a = a_add(&b.attribs, a_new(&at_luacall_building));
  lcbuilding_data * data = (lcbuilding_data*)a->data.v;
  data->b = &b;
  data->fname = strdup(fname);

  return 0;
}

static const char *
building_getinfo(const building& b)
{
  return b.display;
}

static void
building_setinfo(building& b, const char * info)
{
  set_string(&b.display, info);
}

static const char *
building_getname(const building& b)
{
  return b.name;
}

static void
building_setname(building& b, const char * name)
{
  set_string(&b.name, name);
}

static std::ostream& 
operator<<(std::ostream& stream, building& b)
{
  stream << b.name;
  stream << " (" << itoa36(b.no) << ")";
  stream << ", " << b.type->_name;
  stream << " size " << b.size;
  return stream;
}

static bool 
operator==(const building& a, const building&b)
{
  return a.no==b.no;
}

class buildingunit {
public:
  static unit * next(unit * node) { 
    building * b = node->building;
    do {
      node = node->next;
    } while (node !=NULL && node->building!=b);
    return node;
  }
  static unit * value(unit * node) { return node; }
};


static eressea::list<unit *, unit *, buildingunit>
building_units(const building& b) {
  region * r = b.region;
  unit * u = r->units;
  while (u!=NULL && u->building!=&b) u=u->next;
  return eressea::list<unit *, unit *, buildingunit>(u);
}


void
bind_building(lua_State * L) 
{
  at_register(&at_luacall_building);
  module(L)[
    def("get_building", &findbuilding),
    def("add_building", &add_building),

    class_<struct building>("building")
    .def(tostring(self))
    .def(self == building())
    .property("name", &building_getname, &building_setname)
    .property("info", &building_getinfo, &building_setinfo)
    .property("units", &building_units, return_stl_iterator)
    .def_readonly("region", &building::region)
    .def_readonly("id", &building::no)
    .def_readwrite("size", &building::size)
    .def("add_action", &building_addaction)
  ];
}
