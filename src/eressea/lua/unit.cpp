#include <config.h>
#include <eressea.h>

// kernel includes
#include <kernel/region.h>
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/skill.h>
#include <kernel/unit.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

static unit *
add_unit(faction * f, region * r)
{
  if (f->units==NULL) return addplayer(r, f);
  return createunit(r, f, 0, f->race);
}

static void
unit_setnumber(unit& u, int number)
{
  if (u.number==0) {
    set_number(&u, number);
    u.hp = unit_max_hp(&u) * number;
  }
}

static int
unit_getnumber(const unit& u)
{
  return u.number;
}

static int
unit_getitem(const unit& u, const char * iname)
{
  const item_type * itype = it_find(iname);
  if (itype!=NULL) {
    return i_get(u.items, itype);
  }
  return -1;
}

static int
unit_additem(unit& u, const char * iname, int number)
{
  const item_type * itype = it_find(iname);
  if (itype!=NULL) {
    item * i = i_change(&u.items, itype, number);
    return i?i->number:0;
  } // if (itype!=NULL)
  return -1;
}

static int
unit_getskill(const unit& u, const char * skname)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    skill * sv = get_skill(&u, sk);
    if (sv==NULL) return 0;
    return sv->level;
  }
  return -1;
}

static int
unit_effskill(const unit& u, const char * skname)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    return effskill(&u, sk);
  }
  return -1;
}

static int
unit_setskill(unit& u, const char * skname, int level)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    set_level(&u, sk, level);
    return level;
  } // if (sk!=NULL)
  return -1;
}

void
bind_unit(lua_State * L) 
{
  module(L)[
    def("get_unit", &findunit),
    def("add_unit", &add_unit),

    class_<struct unit>("unit")
    .def_readonly("name", &unit::name)
    .def_readonly("region", &unit::region)
    .def_readonly("faction", &unit::faction)
    .def_readonly("id", &unit::no)
    .def("get_item", &unit_getitem)
    .def("add_item", &unit_additem)
    .def("get_skill", &unit_getskill)
    .def("eff_skill", &unit_effskill)
    .def("set_skill", &unit_setskill)
    .property("number", &unit_getnumber, &unit_setnumber)
  ];
}
