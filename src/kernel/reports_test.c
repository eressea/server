#include <platform.h>

#include <kernel/building.h>
#include <kernel/reports.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <cutest/CuTest.h>
#include <tests.h>

static void test_reorder_units(CuTest * tc)
{
  region *r;
  building *b;
  ship * s;
  unit *u0, *u1, *u2, *u3, *u4;
  struct faction * f;
  const building_type *btype;
  const ship_type *stype;

  test_cleanup();
  test_create_world();

  btype = bt_find("castle");
  stype = st_find("boat");

  r = findregion(-1, 0);
  b = test_create_building(r, btype);
  s = test_create_ship(r, stype);
  f = test_create_faction(0);

  u0 = test_create_unit(f, r);
  u_set_ship(u0, s);
  u1 = test_create_unit(f, r);
  u_set_ship(u1, s);
  ship_set_owner(s, u1);
  u2 = test_create_unit(f, r);
  u3 = test_create_unit(f, r);
  u_set_building(u3, b);
  u4 = test_create_unit(f, r);
  u_set_building(u4, b);
  building_set_owner(b, u4);

  reorder_units(r);

  CuAssertPtrEquals(tc, u4, r->units);
  CuAssertPtrEquals(tc, u3, u4->next);
  CuAssertPtrEquals(tc, u2, u3->next);
  CuAssertPtrEquals(tc, u1, u2->next);
  CuAssertPtrEquals(tc, u0, u1->next);
  CuAssertPtrEquals(tc, 0, u0->next);
}

CuSuite *get_reports_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_reorder_units);
  return suite;
}
