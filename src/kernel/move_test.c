#include <kernel/building.h>
#include <kernel/move.h>
#include <kernel/region.h>

#include <util/language.h>

#include <cutest/CuTest.h>
#include <tests.h>

static void building_type_exists(CuTest * tc)
{
  region *r;
  building *b;
  building_type *btype;
  building_type *btype2 = calloc(1, sizeof(building_type));

  test_cleanup();
  test_create_world();

  btype = bt_find("castle");

  r = findregion(-1, 0);
  b = new_building(btype, r, default_locale);

  CuAssertTrue(tc, buildingtype_exists(r, NULL, true) == false);
  CuAssertTrue(tc, buildingtype_exists(r, btype, true) == true);
  CuAssertTrue(tc, buildingtype_exists(r, btype2, true) == false);
}

CuSuite *get_move_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, building_type_exists);
  return suite;
}
