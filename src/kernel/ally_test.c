#include <platform.h>
#include "types.h"
#include "ally.h"

#include <CuTest.h>
#include <tests.h>

static void test_ally(CuTest * tc)
{
  ally * al = 0;
  struct faction * f1 = test_create_faction(0);

  ally_add(&al, f1);
  CuAssertPtrNotNull(tc, al);
  CuAssertPtrEquals(tc, f1, ally_find(al, f1)->faction);

  ally_remove(&al, f1);
  CuAssertPtrEquals(tc, 0, al);
  CuAssertPtrEquals(tc, 0, ally_find(al, f1));
}

CuSuite *get_ally_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_ally);
  return suite;
}

