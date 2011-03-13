#include <kernel/types.h>
#include <platform.h>
#include "battle.h"
#include "building.h"
#include "faction.h"
#include "race.h"
#include "region.h"
#include "unit.h"
#include "tests.h"
#include <cutest/CuTest.h>

static int add_two(building * b, unit * u) {
  return 2;
}

static void test_building_bonus(CuTest * tc)
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
  btype = bt_find("castle");
  btype->protection = &add_two;
  bld = test_create_building(r, btype);
  bld->size = 10;

  du = test_create_unit(test_create_faction(rc_find("human")), r);
  du->building = bld;

  au = test_create_unit(test_create_faction(rc_find("human")), r);

  b = make_battle(r);
  ds = make_side(b, du->faction, 0, 0, 0);
  df = make_fighter(b, du, ds, false);
  CuAssertPtrEquals(tc, bld, df->building);

  as = make_side(b, au->faction, 0, 0, 0);
  af = make_fighter(b, au, as, true);
  CuAssertPtrEquals(tc, 0, af->building);

  dt.fighter = df;
  dt.index = 0;
  at.fighter = af;
  at.index = 0;

  diff = skilldiff(at, dt, 0);
  CuAssertIntEquals(tc, -2, diff);
}

CuSuite *get_battle_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_building_bonus);
  return suite;
}
