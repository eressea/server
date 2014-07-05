#include <platform.h>
#include <stdlib.h>

#include <kernel/building.h>
#include <kernel/move.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

static void test_ship_not_allowed_in_coast(CuTest * tc)
{
  region *r1, *r2;
  ship * sh;
  terrain_type *ttype, *otype;
  ship_type *stype;

  test_cleanup();
  test_create_world();

  ttype = test_create_terrain("glacier", LAND_REGION|ARCTIC_REGION|WALK_INTO|SAIL_INTO);
  otype = test_create_terrain("ocean", SEA_REGION|SAIL_INTO);
  stype = test_create_shiptype("derp");
  stype->coasts = (const struct terrain_type **)calloc(2, sizeof(const struct terrain_type *));

  r1 = test_create_region(0, 0, ttype);
  r2 = test_create_region(1, 0, otype);
  sh = test_create_ship(0, stype);

  CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(sh, r2));
  CuAssertIntEquals(tc, SA_NO_COAST, check_ship_allowed(sh, r1));
  stype->coasts[0] = ttype;
  CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(sh, r1));
}

static void test_ship_allowed_with_harbor(CuTest * tc)
{
  region *r;
  ship * sh;
  terrain_type * ttype;
  building_type * btype;
  building * b;

  test_cleanup();
  test_create_world();

  ttype = test_create_terrain("glacier", LAND_REGION|ARCTIC_REGION|WALK_INTO|SAIL_INTO);
  btype = test_create_buildingtype("harbour");

  r = test_create_region(0, 0, ttype);
  sh = test_create_ship(0, 0);

  b = test_create_building(r, btype);
  b->flags |= BLD_WORKING;
  CuAssertIntEquals(tc, SA_HARBOUR, check_ship_allowed(sh, r));
}

static void test_building_type_exists(CuTest * tc)
{
  region *r;
  building *b;
  building_type *btype, *btype2;

  test_cleanup();
  test_create_world();

  btype2 = bt_get_or_create("lighthouse");
  btype = bt_get_or_create("castle");

  r = findregion(-1, 0);
  b = new_building(btype, r, default_locale);

  CuAssertPtrNotNull(tc, b);
  CuAssertTrue(tc, !buildingtype_exists(r, NULL, false));
  CuAssertTrue(tc, buildingtype_exists(r, btype, false));
  CuAssertTrue(tc, !buildingtype_exists(r, btype2, false));
}

CuSuite *get_move_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_building_type_exists);
  SUITE_ADD_TEST(suite, test_ship_not_allowed_in_coast);
  SUITE_ADD_TEST(suite, test_ship_allowed_with_harbor);
  return suite;
}
