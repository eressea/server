#include <platform.h>

#include <kernel/types.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <cutest/CuTest.h>
#include <tests.h>
#include <stdlib.h>
#include <string.h>

static void test_register_ship(CuTest * tc)
{
  ship_type *stype;

  test_cleanup();

  stype = (ship_type *)calloc(sizeof(ship_type), 1);
  stype->name[0] = strdup("herp");
  st_register(stype);

  CuAssertPtrNotNull(tc, st_find("herp"));
}

static void test_shipowner(CuTest * tc)
{
  struct region *r;
  struct ship * sh;
  struct unit * u;
  struct faction * f;
  const struct ship_type *stype;
  const struct race *human;

  test_cleanup();
  test_create_world();

  human = rc_find("human");
  CuAssertPtrNotNull(tc, human);

  stype = st_find("boat");
  CuAssertPtrNotNull(tc, stype);

  f = test_create_faction(human);
  r = findregion(0, 0);

  sh = test_create_ship(r, stype);
  CuAssertPtrNotNull(tc, sh);

  u = test_create_unit(f, r);
  CuAssertPtrNotNull(tc, u);
  u_set_ship(u, sh);
  CuAssertPtrEquals(tc, u, shipowner(sh));
}

CuSuite *get_ship_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_register_ship);
  SUITE_ADD_TEST(suite, test_shipowner);
  return suite;
}
