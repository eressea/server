#include <config.h>
#include <kernel/eressea.h>
#include "list.h"
#include "objects.h"
#include "bindings.h"
#include "event.h"

// Atributes includes
#include <attributes/racename.h>
#include <attributes/key.h>

// kernel includes
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/move.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

// util includes
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/lists.h>
#include <util/log.h>

// lua includes
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4127)
#endif
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#if LUABIND_BETA>=7
# include <luabind/operator.hpp>
#endif
#ifdef _MSC_VER
#pragma warning (pop)
#endif

#include <ostream>
#include <string>
using namespace luabind;

class bind_spell_ptr {
public:
  static spell_list * next(spell_list * node) { return node->next; }
  static spell * value(spell_list * node) { return node->data; }
};

static eressea::list<spell *, spell_list *, bind_spell_ptr>
unit_spells(const unit * u) {
  sc_mage * mage = get_mage(u);
  if (mage==NULL) return eressea::list<spell *, spell_list *, bind_spell_ptr>(NULL);
  spell_list * splist = mage->spells;
  return eressea::list<spell *, spell_list *, bind_spell_ptr>(splist);
}

class bind_spell_list {
public:
  static spell_list * next(spell_list * node) { return node->next; }
  static spell * value(spell_list * node) { return node->data; }
};

class bind_orders {
public:
  static order * next(order * node) { return node->next; }
  static std::string value(order * node) { 
    char * cmd = (char*)getcommand(node); 
    std::string s(cmd);
    free(cmd);
    return s;
  }
};

static eressea::list<std::string, order *, bind_orders>
unit_orders(const unit * u) {
  return eressea::list<std::string, order *, bind_orders>(u->orders);
}

static eressea::list<std::string, item *, eressea::bind_items>
unit_items(const unit * u) {
  return eressea::list<std::string, item *, eressea::bind_items>(u->items);
}

static unit *
add_unit(faction * f, region * r)
{
  if (f->units==NULL) return addplayer(r, f);
  return createunit(r, f, 0, f->race);
}

static void
unit_setnumber(unit * u, int number)
{
  if (u->number==0) {
    set_number(u, number);
    u->hp = unit_max_hp(u) * number;
  } else {
    scale_number(u, number);
  }
}

static void
unit_setracename(unit * u, const char * name)
{
  set_racename(&u->attribs, name);
}

static int
unit_getnumber(const unit * u)
{
  return u->number;
}

static int
unit_getitem(const unit * u, const char * iname)
{
  if (iname!=NULL) {
    const item_type * itype = it_find(iname);
    if (itype!=NULL) {
      return i_get(u->items, itype);
    }
  }
  return -1;
}

static int
unit_additem(unit * u, const char * iname, int number)
{
  if (iname!=NULL) {
    const item_type * itype = it_find(iname);
    if (itype!=NULL) {
      item * i = i_change(&u->items, itype, number);
      return i?i->number:0;
    } // if (itype!=NULL)
  }
  const item_type * itype = it_find(iname);
  if (itype!=NULL) {
    item * i = i_change(&u->items, itype, number);
    return i?i->number:0;
  }
  return -1;
}

static int
unit_usepooled(unit * u, const char * iname, int number)
{
  const resource_type * rtype = rt_find(iname);
  if (rtype!=NULL) {
    return use_pooled(u, rtype, GET_DEFAULT, number);
  }
  return -1;
}

static int
unit_getpooled(unit * u, const char * iname)
{
  const resource_type * rtype = rt_find(iname);
  if (rtype!=NULL) {
    return get_pooled(u, rtype, GET_DEFAULT, INT_MAX);
  }
  return -1;
}

static int
unit_getskill(const unit * u, const char * skname)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    skill * sv = get_skill(u, sk);
    if (sv==NULL) return 0;
    return sv->level;
  }
  return -1;
}

static int
unit_effskill(const unit * u, const char * skname)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    return effskill(u, sk);
  }
  return -1;
}

static int
unit_setskill(unit * u, const char * skname, int level)
{
  skill_t sk = sk_find(skname);
  if (sk!=NOSKILL) {
    set_level(u, sk, level);
    return level;
  } // if (sk!=NULL)
  return -1;
}

static const char *
unit_getrace(const unit * u)
{
  return u->race->_name[0];
}

static void
unit_setrace(unit * u, const char * rcname)
{
  race * rc = rc_find(rcname);
  if (rc!=NULL) {
    if (u->irace==u->race) u->irace = rc;
    u->race = rc;
  }
}

static void
unit_castspell(unit * u, const char * name)
{
  spell_list * slist = spells;
  while (slist!=NULL) {
    spell * sp = slist->data;
    if (strcmp(name, sp->sname)==0) {
      castorder * co = (castorder*)malloc(sizeof(castorder));
      co->distance = 0;
      co->familiar = NULL;
      co->force = sp->level;
      co->level = sp->level;
      co->magician.u = u;
      co->order = NULL;
      co->par = NULL;
      co->rt = u->region;
      co->sp = sp;
      if (sp->sp_function==NULL) {
        log_error(("spell '%s' has no function.\n", sp->sname));
        co->level = 0;
      } else {
        sp->sp_function(co);
      }
      free(co);
    }
    slist=slist->next;
  }
}

static void
unit_addspell(unit * u, const char * name)
{
  bool add = false;
  spell_list * slist = spells;
  while (slist!=NULL) {
    spell * sp = slist->data;
    if (strcmp(name, sp->sname)==0) {
      struct sc_mage * mage = get_mage(u);
      if (add) log_error(("two spells are called %s.\n", name));
      add_spell(mage, sp);
      add = true;
    }
    slist=slist->next;
  }
  if (!add) log_error(("spell %s could not be found\n", name));
}

static unit *
unit_isfamiliar(const unit * u)
{
  attrib * a = a_find(u->attribs, &at_familiarmage);
  if (a!=NULL) {
    return (unit*)a->data.v;
  }
  return NULL;
}

static unit *
unit_getfamiliar(const unit * u)
{
  attrib * a = a_find(u->attribs, &at_familiar);
  if (a!=NULL) {
    return (unit*)a->data.v;
  }
  return NULL;
}

static void
unit_setfamiliar(unit * mage, unit * familiar)
{
  create_newfamiliar(mage, familiar);
}

static void
unit_removespell(unit * u, const spell * sp)
{
  sc_mage * mage = get_mage(u);
  if (mage!=NULL) {
    spell_list ** isptr = &mage->spells;
    while (*isptr && (*isptr)->data != sp) {
      isptr = &(*isptr)->next;
    }
    if (*isptr) {
      spell_list * sptr = *isptr;
      *isptr = sptr->next;
      free(sptr);
    }
  }
}

static int
unit_hpmax(const unit * u)
{
  return unit_max_hp(u);
}

static void
unit_setregion(unit * u, region * r)
{
  move_unit(u, r, NULL);
}

static region *
unit_getregion(const unit * u)
{
  return u->region;
}

static void
unit_setship(unit * u, ship * s)
{
  leave(u->region, u);
  if (u->region!=s->region) {
    move_unit(u, s->region, NULL);
  }
  u->ship = s;
}

static ship *
unit_getship(const unit * u)
{
  return u->ship;
}

static void
unit_setbuilding(unit * u, building * b)
{
  leave(u->region, u);
  if (u->region!=b->region) {
    move_unit(u, b->region, NULL);
  }
  u->building = b;
}

static building *
unit_getbuilding(const unit * u)
{
  return u->building;
}

static int
unit_getid(const unit * u)
{
  return u->no;
}

static void
unit_setid(unit * u, int id)
{
  unit * nu = findunit(id);
  if (nu==NULL) {
    uunhash(u);
    u->no = id;
    uhash(u);
  }
}

static const char *
unit_getname(const unit * u)
{
  return (const char *)u->name;
}

static void
unit_setname(unit * u, const char * name)
{
  free(u->name);
  u->name = strdup(name);
}

static const char *
unit_getinfo(const unit * u)
{
  return (const char *)u->display;
}

static void
unit_setinfo(unit * u, const char * info)
{
  free(u->display);
  u->display = strdup(info);
}

static bool
get_flag(const unit * u, const char * name)
{
  int flag = atoi36(name);
  attrib * a = find_key(u->attribs, flag);
  return (a!=NULL);
}

static void
set_flag(unit * u, const char * name, bool value)
{
  int flag = atoi36(name);
  attrib * a = find_key(u->attribs, flag);
  if (a==NULL && value) {
    add_key(&u->attribs, flag);
  } else if (a!=NULL && !value) {
    a_remove(&u->attribs, a);
  }
}

static std::ostream& 
operator<<(std::ostream& stream, const unit& u)
{
  const char * rcname = get_racename(u.attribs);
  stream << u.name << " (" << itoa36(u.no) << "), " << u.number << " " << u.race->_name[0];
  if (rcname) stream << "/" << rcname;
  return stream;
}

static bool 
operator==(const unit& a, const unit &b)
{
  return a.no==b.no;
}

static int
unit_getaura(const unit * u)
{
  return get_spellpoints(u);
}

static void
unit_setaura(unit * u, int points)
{
  return set_spellpoints(u, points);
}

static faction *
unit_getfaction(const unit * u)
{
  return u->faction;
}

static void
unit_setfaction(unit * u, faction * f)
{
  u_setfaction(u, f);
}

static const char * 
unit_getmagic(const unit * u)
{
  sc_mage * mage = get_mage(u);
  return mage?magietypen[mage->magietyp]:NULL;
}

static void
unit_setmagic(unit * u, const char * type)
{
  sc_mage * mage = get_mage(u);
  magic_t mtype;
  for (mtype=0;mtype!=MAXMAGIETYP;++mtype) {
    if (strcmp(magietypen[mtype], type)==0) break;
  }
  if (mtype==MAXMAGIETYP) return;
  if (mage==NULL) {
    mage = create_mage(u, mtype);
  }
}

static void
unit_addorder(unit * u, const char * str)
{
  order * ord = parse_order(str, u->faction->locale);
  addlist(&u->orders, ord);
  u->faction->lastorders = turn;
}

static void
unit_clearorders(unit * u)
{
  free_orders(&u->orders);
}

static int
unit_weight(const struct unit * u)
{
  return weight(u);
}

typedef struct fctr_data {
  unit * target;
  luabind::object * fptr;
} fctr_data;

#include <exception>

int
fctr_handle(struct trigger * tp, void * data)
{
  trigger * t = tp;
  event * evt = new event(NULL, (event_arg*)data);
  fctr_data * fd = (fctr_data*)t->data.v;
  try {	
    fd->fptr->operator()(fd->target, evt);
  }
  catch (luabind::error& e) {
    lua_State* L = e.state();
    const char* error = lua_tostring(L, -1);
    log_error((error));
    lua_pop(L, 1);
    std::terminate();
  }

  delete evt;
  return 0;
}

static void
fctr_init(trigger * t)
{
  t->data.v = calloc(sizeof(fctr_data), 1);
}

static void
fctr_done(trigger * t)
{
  free(t->data.v);
}

static struct trigger_type tt_functor = {
  "functor",
  fctr_init,
  fctr_done,
  fctr_handle
};

static trigger *
trigger_functor(struct unit * u, const object& f)
{
  luabind::object * fptr = new luabind::object(f);
  trigger * t = t_new(&tt_functor);
  fctr_data * td = (fctr_data*)t->data.v;
  td->target = u;
  td->fptr = fptr;
  return t;
}

static void
unit_addhandler(struct unit * u, const char * event, const object& f)
{
  add_trigger(&u->attribs, event, trigger_functor(u, f));
}

static int
unit_capacity(const struct unit * u)
{
  return walkingcapacity(u);
}

static void
unit_addnotice(unit * u, const char * str)
{
  addmessage(u->region, u->faction, str, MSG_MESSAGE, ML_IMPORTANT);
}

void
bind_unit(lua_State * L) 
{
  module(L)[
    def("get_unit", &findunit),
    def("add_unit", &add_unit),

    class_<struct unit>("unit")
    .def(tostring(self))
    .def(self == unit())
    .property("name", &unit_getname, &unit_setname)
    .property("info", &unit_getinfo, &unit_setinfo)
    .property("id", &unit_getid, &unit_setid)
    .property("faction", &unit_getfaction, &unit_setfaction)
    .def_readwrite("hp", &unit::hp)
    .def_readwrite("status", &unit::status)
    .property("weight", &unit_weight)
    .property("capacity", &unit_capacity)

    .property("is_familiar", &unit_isfamiliar)
    .property("familiar", &unit_getfamiliar, &unit_setfamiliar)

    // orders:
    .def("add_order", &unit_addorder)
    .def("clear_orders", &unit_clearorders)
    .property("orders", &unit_orders, return_stl_iterator)

    // key-attributes for named flags:
    .def("set_flag", &set_flag)
    .def("get_flag", &get_flag)
    .def_readwrite("flags", &unit::flags)
    .def_readwrite("age", &unit::age)

    // items:
    .def("get_item", &unit_getitem)
    .def("add_item", &unit_additem)
    .property("items", &unit_items, return_stl_iterator)

    .def("get_pooled", &unit_getpooled)
    .def("use_pooled", &unit_usepooled)

    // skills:
    .def("get_skill", &unit_getskill)
    .def("eff_skill", &unit_effskill)
    .def("set_skill", &unit_setskill)

    .def("add_notice", &unit_addnotice)

    // npc logic:
    .def("add_handler", &unit_addhandler)

    .def("set_racename", &unit_setracename)
    .def("add_spell", &unit_addspell)
    .def("remove_spell", &unit_removespell)
    .def("cast_spell", &unit_castspell)

    .property("magic", &unit_getmagic, &unit_setmagic)
    .property("aura", &unit_getaura, &unit_setaura)
    .property("building", &unit_getbuilding, &unit_setbuilding)
    .property("ship", &unit_getship, &unit_setship)
    .property("region", &unit_getregion, &unit_setregion)
    .property("spells", &unit_spells, return_stl_iterator)
    .property("number", &unit_getnumber, &unit_setnumber)
    .property("race", &unit_getrace, &unit_setrace)
    .property("hp_max", &unit_hpmax)
    .property("objects", &eressea::get_objects<unit>)
  ];
}
