#include <platform.h>

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

static void test_resources(CuTest *tc) {
    const char *names[] = { "horse", "horse_p" };
    resource_type *rtype;
    test_cleanup();
    CuAssertPtrEquals(tc, 0, rt_find("horse"));
    rtype = new_resourcetype(names, 0, 0);
    CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_HORSE));
    rt_register(rtype);
    CuAssertPtrEquals(tc, (void *)rtype, (void *)rt_find("horse"));
    CuAssertPtrEquals(tc, (void *)rtype, (void *)get_resourcetype(R_HORSE));
    test_cleanup();
    CuAssertPtrEquals(tc, 0, rt_find("horse"));
    rtype = new_resourcetype(names, 0, 0);
    CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_HORSE));
    rt_register(rtype);
    CuAssertPtrEquals(tc, (void *)rtype, (void *)get_resourcetype(R_HORSE));
}

static void test_recreate_world(CuTest * tc)
{
  test_cleanup();
  CuAssertPtrEquals(tc, 0, get_locale("de"));
  CuAssertPtrEquals(tc, 0, it_find("horse"));
  CuAssertPtrNotNull(tc, get_resourcetype(R_LIFE));
  CuAssertPtrNotNull(tc, get_resourcetype(R_PERMAURA));
  CuAssertPtrNotNull(tc, get_resourcetype(R_AURA));
  CuAssertPtrNotNull(tc, it_find("money"));

  test_create_world();
  CuAssertPtrEquals(tc, default_locale, get_locale("de"));
  CuAssertPtrNotNull(tc, default_locale);
  CuAssertPtrNotNull(tc, findregion(0, 0));
  CuAssertPtrNotNull(tc, it_find("horse"));
  CuAssertPtrNotNull(tc, get_resourcetype(R_HORSE));
  CuAssertPtrNotNull(tc, it_find("money"));
  CuAssertPtrNotNull(tc, get_resourcetype(R_LIFE));
  CuAssertPtrNotNull(tc, get_resourcetype(R_SILVER));
  CuAssertPtrNotNull(tc, get_resourcetype(R_AURA));
  CuAssertPtrNotNull(tc, get_resourcetype(R_PERMAURA));
  CuAssertPtrNotNull(tc, get_resourcetype(R_PEASANT));
  CuAssertPtrNotNull(tc, get_resourcetype(R_UNIT));

  test_cleanup();
  CuAssertPtrEquals(tc, 0, get_locale("de"));
  CuAssertPtrEquals(tc, 0, (void*)it_find("horse"));
  CuAssertPtrEquals(tc, 0, (void*)get_resourcetype(R_HORSE));
  CuAssertPtrNotNull(tc, it_find("money"));
  CuAssertPtrNotNull(tc, get_resourcetype(R_LIFE));
  CuAssertPtrNotNull(tc, get_resourcetype(R_SILVER));
  CuAssertPtrNotNull(tc, get_resourcetype(R_AURA));
  CuAssertPtrNotNull(tc, get_resourcetype(R_PERMAURA));
  CuAssertPtrNotNull(tc, get_resourcetype(R_PEASANT));
  CuAssertPtrNotNull(tc, get_resourcetype(R_UNIT));
  CuAssertPtrEquals(tc, 0, findregion(0, 0));
}

CuSuite *get_tests_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_resources);
  SUITE_ADD_TEST(suite, test_recreate_world);
  return suite;
}
