#include <util/language.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <CuTest.h>

static void test_recreate_world(CuTest * tc)
{
  test_cleanup();
  CuAssertPtrEquals(tc, 0, find_locale("de"));
  CuAssertPtrEquals(tc, 0, it_find("money"));
  CuAssertPtrEquals(tc, 0, it_find("horse"));

  test_create_world();
  CuAssertPtrEquals(tc, default_locale, find_locale("de"));
  CuAssertPtrNotNull(tc, default_locale);
  CuAssertPtrNotNull(tc, findregion(0, 0));
  CuAssertPtrNotNull(tc, it_find("money"));
  CuAssertPtrNotNull(tc, it_find("horse"));
  CuAssertPtrNotNull(tc, rt_find("horse"));
  CuAssertPtrNotNull(tc, rt_find("hp"));
  CuAssertPtrNotNull(tc, rt_find("money"));
  CuAssertPtrNotNull(tc, rt_find("aura"));
  CuAssertPtrNotNull(tc, rt_find("permaura"));
  CuAssertPtrNotNull(tc, rt_find("peasant"));
  CuAssertPtrNotNull(tc, rt_find("unit"));

  test_cleanup();
  CuAssertPtrEquals(tc, 0, find_locale("de"));
  CuAssertPtrEquals(tc, 0, it_find("money"));
  CuAssertPtrEquals(tc, 0, it_find("horse"));
  CuAssertPtrEquals(tc, 0, rt_find("horse"));
  CuAssertPtrEquals(tc, 0, rt_find("hp"));
  CuAssertPtrEquals(tc, 0, rt_find("money"));
  CuAssertPtrEquals(tc, 0, rt_find("aura"));
  CuAssertPtrEquals(tc, 0, rt_find("permaura"));
  CuAssertPtrEquals(tc, 0, rt_find("peasant"));
  CuAssertPtrEquals(tc, 0, rt_find("unit"));
  CuAssertPtrEquals(tc, 0, findregion(0, 0));
}

CuSuite *get_tests_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_recreate_world);
  return suite;
}
