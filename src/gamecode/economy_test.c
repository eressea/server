#include "platform.h"
#include "economy.h"

#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/building.h>
#include <kernel/ship.h>

#include <cutest/CuTest.h>
#include <tests.h>

static void test_give_control_building(CuTest * tc)
{
  unit *u1, *u2;
  building *b;
  struct faction *f;
  region *r;

  test_cleanup();
  test_create_world();
  f = test_create_faction(0);
  r = findregion(0, 0);
  b = test_create_building(r, 0);
  u1 = test_create_unit(f, r);
  u_set_building(u1, b);
  u2 = test_create_unit(f, r);
  u_set_building(u2, b);
  CuAssertPtrEquals(tc, u1, building_owner(b));
  give_control(u1, u2);
  CuAssertPtrEquals(tc, u2, building_owner(b));
}

static void test_give_control_ship(CuTest * tc)
{
  unit *u1, *u2;
  ship *sh;
  struct faction *f;
  region *r;

  test_cleanup();
  test_create_world();
  f = test_create_faction(0);
  r = findregion(0, 0);
  sh = test_create_ship(r, 0);
  u1 = test_create_unit(f, r);
  u_set_ship(u1, sh);
  u2 = test_create_unit(f, r);
  u_set_ship(u2, sh);
  CuAssertPtrEquals(tc, u1, ship_owner(sh));
  give_control(u1, u2);
  CuAssertPtrEquals(tc, u2, ship_owner(sh));
}

CuSuite *get_economy_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_give_control_building);
  SUITE_ADD_TEST(suite, test_give_control_ship);
  return suite;
}
