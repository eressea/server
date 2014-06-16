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
  region *r;
  ship * sh;
  terrain_type * ttype;
  ship_type * stype;
  const char * names[] = { "derp", "derp_p" };

  test_cleanup();
  test_create_world();

  ttype = test_create_terrain("glacier", LAND_REGION|ARCTIC_REGION|WALK_INTO|SAIL_INTO);
  stype = test_create_shiptype(names);
  stype->coasts = (const struct terrain_type **)calloc(2, sizeof(const struct terrain_type *));

  r = test_create_region(0, 0, ttype);
  sh = test_create_ship(0, stype);

  CuAssertIntEquals(tc, SA_NO_COAST, check_ship_allowed(sh, r));
  stype->coasts[0] = ttype;
  CuAssertIntEquals(tc, SA_COAST, check_ship_allowed(sh, r));
}

static void test_ship_allowed_with_harbor(CuTest * tc)
{
  region *r;
  ship * sh;
  terrain_type * ttype;
  building_type * btype;

  test_cleanup();
  test_create_world();

  ttype = test_create_terrain("glacier", LAND_REGION|ARCTIC_REGION|WALK_INTO|SAIL_INTO);
  btype = test_create_buildingtype("harbour");

  r = test_create_region(0, 0, ttype);
  sh = test_create_ship(0, 0);

  test_create_building(r, btype);
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
  CuAssertTrue(tc, !buildingtype_exists(r, NULL, true));
  CuAssertTrue(tc, buildingtype_exists(r, btype, true));
  CuAssertTrue(tc, !buildingtype_exists(r, btype2, true));
}

CuSuite *get_move_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_building_type_exists);
  SUITE_ADD_TEST(suite, test_ship_not_allowed_in_coast);
  SUITE_ADD_TEST(suite, test_ship_allowed_with_harbor);
  return suite;
}
