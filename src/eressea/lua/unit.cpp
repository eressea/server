#include <config.h>
#include <eressea.h>
#include "list.h"

// Atributes includes
#include <attributes/racename.h>

// kernel includes
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/skill.h>
#include <kernel/unit.h>
#include <kernel/magic.h>
#include <kernel/spell.h>

// lua includes
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

using namespace luabind;

class bind_spell_ptr {
public:
  static spell_ptr * next(spell_ptr * node) { return node->next; }
  static spell * value(spell_ptr * node) { return find_spellbyid(node->spellid); }
};

static eressea::list<spell, spell_ptr, bind_spell_ptr>
unit_spells(const unit& u) {
  sc_mage * mage = get_mage(&u);
  if (mage==NULL) return eressea::list<spell, spell_ptr, bind_spell_ptr>(NULL);
  spell_ptr * splist = mage->spellptr;
  return eressea::list<spell, spell_ptr, bind_spell_ptr>(splist);
}

class bind_spell_list {
public:
  static spell_list * next(spell_list * node) { return node->next; }
  static spell * value(spell_list * node) { return node->data; }
};

static eressea::list<spell, spell_list, bind_spell_list>
unit_familiarspells(const unit& u) {
  spell_list * spells = familiarspells(u.race);
  return eressea::list<spell, spell_list, bind_spell_list>(spells);
}

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
  } else {
    scale_number(&u, number);
  }
}

static void
unit_setracename(unit& u, const char * name)
{
  set_racename(&u.attribs, name);
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

static const char *
unit_getrace(const unit& u)
{
  return u.race->_name[0];
}

static void
unit_setrace(unit& u, const char * rcname)
{
  race * rc = rc_find(rcname);
  if (rc!=NULL) {
    u.race = rc;
  }
}

static void
unit_addspell(unit& u, const char * name)
{
  bool add = false;
  spell_list * slist = spells;
  while (slist!=NULL) {
    spell * sp = slist->data;
    if (strcmp(name, sp->sname)==0) {
      if (add) log_error(("two spells are called %s.\n", name));
      addspell(&u, sp->id);
      add = true;
    }
    slist=slist->next;
  }
  if (!add) log_error(("spell %s could not be found\n", name));
}

static bool
unit_isfamiliar(const unit& u)
{
  return is_familiar(&u)!=0;
}

static void
unit_removespell(unit& u, const spell * sp)
{
  sc_mage * mage = get_mage(&u);
  if (mage!=NULL) {
    spell_ptr ** isptr = &mage->spellptr;
    while (*isptr && (*isptr)->spellid != sp->id) {
      isptr = &(*isptr)->next;
    }
    if (*isptr) {
      spell_ptr * sptr = *isptr;
      *isptr = sptr->next;
      free(sptr);
    }
  }
}

static int
unit_hpmax(const unit& u)
{
  return unit_max_hp(&u);
}

static void
unit_setregion(unit& u, region& r)
{
  move_unit(&u, &r, NULL);
}

static region *
unit_getregion(const unit& u)
{
  return u.region;
}

void
bind_unit(lua_State * L) 
{
  module(L)[
    def("get_unit", &findunit),
    def("add_unit", &add_unit),

    class_<struct unit>("unit")
    .def_readwrite("name", &unit::name)
    .def_readonly("faction", &unit::faction)
    .def_readonly("id", &unit::no)
    .def_readwrite("hp", &unit::hp)
    .def("get_item", &unit_getitem)
    .def("add_item", &unit_additem)
    .def("get_skill", &unit_getskill)
    .def("eff_skill", &unit_effskill)
    .def("set_skill", &unit_setskill)
    .def("set_racename", &unit_setracename)
    .def("add_spell", &unit_addspell)
    .def("remove_spell", &unit_removespell)
    .property("region", &unit_getregion, &unit_setregion)
    .property("is_familiar", &unit_isfamiliar)
    .property("spells", &unit_spells, return_stl_iterator)
    .property("familiarspells", &unit_familiarspells, return_stl_iterator)
    .property("number", &unit_getnumber, &unit_setnumber)
    .property("race", &unit_getrace, &unit_setrace)
    .property("hp_max", &unit_hpmax)
  ];
}
