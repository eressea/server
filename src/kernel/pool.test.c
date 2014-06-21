#include <platform.h>
#include <kernel/types.h>

#include "pool.h"
#include "magic.h"
#include "unit.h"
#include "item.h"
#include "region.h"
#include "skill.h"

#include <CuTest.h>
#include <tests.h>

void test_change_resource(CuTest * tc)
{
  struct unit * u;
  struct faction * f;
  struct region * r;
  const char * names[] = { "money", "aura", "permaura", "horse", "hp", 0 };
  int i;

  test_cleanup();
  test_create_world();
  enable_skill(SK_MAGIC, true);

  r = findregion(0, 0);
  f = test_create_faction(0);
  u = test_create_unit(f, r);
  CuAssertPtrNotNull(tc, u);
  set_level(u, SK_MAGIC, 5);
  create_mage(u, M_DRAIG);

  for (i=0;names[i];++i) {
    const struct resource_type *rtype = rt_find(names[i]);
    int have = get_resource(u, rtype);
    CuAssertIntEquals(tc, have+1, change_resource(u, rtype, 1));
    CuAssertIntEquals(tc, have+1, get_resource(u, rtype));
  }
}

CuSuite *get_pool_suite(void)
{
  CuSuite *suite = CuSuiteNew();
/*  SUITE_ADD_TEST(suite, test_pool); */
  SUITE_ADD_TEST(suite, test_change_resource);
  return suite;
}
