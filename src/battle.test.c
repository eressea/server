#include <platform.h>

#include "battle.h"
#include "skill.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <CuTest.h>
#include "tests.h"

static void test_make_fighter(CuTest * tc)
{
    unit *au;
    region *r;
    fighter *af;
    battle *b;
    side *as;
    faction * f;
    const resource_type *rtype;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f = test_create_faction(rc_find("human"));
    au = test_create_unit(f, r);
    enable_skill(SK_MAGIC, true);
    enable_skill(SK_RIDING, true);
    set_level(au, SK_MAGIC, 3);
    set_level(au, SK_RIDING, 3);
    au->status = ST_BEHIND;
    rtype = get_resourcetype(R_HORSE);
    i_change(&au->items, rtype->itype, 1);
    
    b = make_battle(r);
    as = make_side(b, au->faction, 0, 0, 0);
    af = make_fighter(b, au, as, false);
    
    CuAssertIntEquals(tc, 1, b->nfighters);
    CuAssertPtrEquals(tc, 0, af->building);
    CuAssertPtrEquals(tc, as, af->side);
    CuAssertIntEquals(tc, 0, af->run.hp);
    CuAssertIntEquals(tc, ST_BEHIND, af->status);
    CuAssertIntEquals(tc, 0, af->run.number);
    CuAssertIntEquals(tc, au->hp, af->person[0].hp);
    CuAssertIntEquals(tc, 1, af->person[0].speed);
    CuAssertIntEquals(tc, au->number, af->alive);
    CuAssertIntEquals(tc, 0, af->removed);
    CuAssertIntEquals(tc, 3, af->magic);
    CuAssertIntEquals(tc, 1, af->horses);
    CuAssertIntEquals(tc, 0, af->elvenhorses);
    free_battle(b);
    test_cleanup();
}

static int add_two(building * b, unit * u) {
  return 2;
}

static void test_defenders_get_building_bonus(CuTest * tc)
{
  unit *du, *au;
  region *r;
  building * bld;
  fighter *df, *af;
  battle *b;
  side *ds, *as;
  int diff;
  troop dt, at;
  building_type * btype;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  btype = bt_get_or_create("castle");
  btype->protection = &add_two;
  bld = test_create_building(r, btype);
  bld->size = 10;

  du = test_create_unit(test_create_faction(rc_find("human")), r);
  au = test_create_unit(test_create_faction(rc_find("human")), r);
  u_set_building(du, bld);

  b = make_battle(r);
  ds = make_side(b, du->faction, 0, 0, 0);
  df = make_fighter(b, du, ds, false);
  as = make_side(b, au->faction, 0, 0, 0);
  af = make_fighter(b, au, as, true);

  CuAssertPtrEquals(tc, bld, df->building);
  CuAssertPtrEquals(tc, 0, af->building);

  dt.fighter = df;
  dt.index = 0;
  at.fighter = af;
  at.index = 0;

  diff = skilldiff(at, dt, 0);
  CuAssertIntEquals(tc, -2, diff);

  diff = skilldiff(dt, at, 0);
  CuAssertIntEquals(tc, 0, diff);
    free_battle(b);
    test_cleanup();
}

static void test_attackers_get_no_building_bonus(CuTest * tc)
{
  unit *au;
  region *r;
  building * bld;
  fighter *af;
  battle *b;
  side *as;
  building_type * btype;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  btype = bt_get_or_create("castle");
  btype->protection = &add_two;
  bld = test_create_building(r, btype);
  bld->size = 10;

  au = test_create_unit(test_create_faction(rc_find("human")), r);
  u_set_building(au, bld);

  b = make_battle(r);
  as = make_side(b, au->faction, 0, 0, 0);
  af = make_fighter(b, au, as, true);

  CuAssertPtrEquals(tc, 0, af->building);
    free_battle(b);
    test_cleanup();
}

static void test_building_bonus_respects_size(CuTest * tc)
{
  unit *au, *du;
  region *r;
  building * bld;
  fighter *af, *df;
  battle *b;
  side *as;
  building_type * btype;
  faction * f;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  btype = bt_get_or_create("castle");
  btype->protection = &add_two;
  bld = test_create_building(r, btype);
  bld->size = 10;

  f = test_create_faction(rc_find("human"));
  au = test_create_unit(f, r);
  scale_number(au, 9);
  u_set_building(au, bld);
  du = test_create_unit(f, r);
  u_set_building(du, bld);
  scale_number(du, 2);

  b = make_battle(r);
  as = make_side(b, au->faction, 0, 0, 0);
  af = make_fighter(b, au, as, false);
  df = make_fighter(b, du, as, false);

  CuAssertPtrEquals(tc, bld, af->building);
  CuAssertPtrEquals(tc, 0, df->building);
    free_battle(b);
    test_cleanup();
}

CuSuite *get_battle_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_make_fighter);
  SUITE_ADD_TEST(suite, test_defenders_get_building_bonus);
  SUITE_ADD_TEST(suite, test_attackers_get_no_building_bonus);
  SUITE_ADD_TEST(suite, test_building_bonus_respects_size);
  return suite;
}
