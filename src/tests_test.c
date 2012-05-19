#include <kernel/item.h>
#include <kernel/region.h>

#include <cutest/CuTest.h>

static void test_recreate_world(CuTest * tc)
{
  test_cleanup();
  CuAssertPtrEquals(tc, 0, it_find("money"));
  CuAssertPtrEquals(tc, 0, it_find("horse"));
  test_create_world();
  CuAssertPtrNotNull(tc, it_find("money"));
  CuAssertPtrNotNull(tc, it_find("horse"));
  CuAssertPtrNotNull(tc, findregion(0, 0));
  test_cleanup();
  CuAssertPtrEquals(tc, 0, it_find("money"));
  CuAssertPtrEquals(tc, 0, it_find("horse"));
  CuAssertPtrEquals(tc, 0, findregion(0, 0));
}

CuSuite *get_tests_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_recreate_world);
  return suite;
}
