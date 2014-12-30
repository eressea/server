#include <platform.h>

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/terrain.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

static void test_resources(CuTest *tc) {
    resource_type *rtype;
    test_cleanup();
    init_resources();
    CuAssertPtrNotNull(tc, rt_find("hp"));
    CuAssertPtrEquals(tc, rt_find("hp"), (void *)get_resourcetype(R_LIFE));
    CuAssertPtrNotNull(tc, rt_find("peasant"));
    CuAssertPtrEquals(tc, rt_find("peasant"), (void *)get_resourcetype(R_PEASANT));
    CuAssertPtrNotNull(tc, rt_find("aura"));
    CuAssertPtrEquals(tc, rt_find("aura"), (void *)get_resourcetype(R_AURA));
    CuAssertPtrNotNull(tc, rt_find("permaura"));
    CuAssertPtrEquals(tc, rt_find("permaura"), (void *)get_resourcetype(R_PERMAURA));

    CuAssertPtrEquals(tc, 0, rt_find("stone"));
    rtype = rt_get_or_create("stone");
    CuAssertPtrEquals(tc, (void *)rtype, (void *)rt_find("stone"));
    CuAssertPtrEquals(tc, (void *)rtype, (void *)get_resourcetype(R_STONE));
    test_cleanup();
    CuAssertPtrEquals(tc, 0, rt_find("stone"));
    CuAssertPtrEquals(tc, 0, rt_find("peasant"));
    rtype = rt_get_or_create("stone");
    CuAssertPtrEquals(tc, (void *)rtype, (void *)get_resourcetype(R_STONE));
}

static void test_recreate_world(CuTest * tc)
{
  test_cleanup();
  CuAssertPtrEquals(tc, 0, get_locale("de"));
  CuAssertPtrEquals(tc, 0, (void *)rt_find("horse"));

  test_create_world();
  CuAssertPtrEquals(tc, default_locale, get_locale("de"));
  CuAssertPtrNotNull(tc, default_locale);
  CuAssertPtrNotNull(tc, findregion(0, 0));
  CuAssertPtrNotNull(tc, get_terrain("plain"));
  CuAssertPtrNotNull(tc, get_terrain("ocean"));
  CuAssertPtrNotNull(tc, (void *)rt_find("horse"));
  CuAssertPtrNotNull(tc, get_resourcetype(R_HORSE));
  CuAssertPtrNotNull(tc, (void *)rt_find("money"));
  CuAssertPtrNotNull(tc, get_resourcetype(R_LIFE));
  CuAssertPtrNotNull(tc, get_resourcetype(R_SILVER));
  CuAssertPtrNotNull(tc, get_resourcetype(R_AURA));
  CuAssertPtrNotNull(tc, get_resourcetype(R_PERMAURA));
  CuAssertPtrNotNull(tc, get_resourcetype(R_PEASANT));

  test_cleanup();
  CuAssertPtrEquals(tc, 0, get_locale("de"));
  CuAssertPtrEquals(tc, 0, (void*)get_terrain("plain"));
  CuAssertPtrEquals(tc, 0, (void*)get_terrain("ocean"));
  CuAssertPtrEquals(tc, 0, (void*)rt_find("horse"));
  CuAssertPtrEquals(tc, 0, (void*)get_resourcetype(R_HORSE));
  CuAssertPtrEquals(tc, 0, (void *)rt_find("money"));
  CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_LIFE));
  CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_SILVER));
  CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_AURA));
  CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_PERMAURA));
  CuAssertPtrEquals(tc, 0, (void *)get_resourcetype(R_PEASANT));
  CuAssertPtrEquals(tc, 0, findregion(0, 0));
}

CuSuite *get_tests_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_resources);
  SUITE_ADD_TEST(suite, test_recreate_world);
  return suite;
}
