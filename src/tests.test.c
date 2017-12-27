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
    test_setup();
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
    free_resources();
    CuAssertPtrEquals(tc, 0, rt_find("stone"));
    CuAssertPtrEquals(tc, 0, rt_find("peasant"));
    rtype = rt_get_or_create("stone");
    CuAssertPtrEquals(tc, (void *)rtype, (void *)get_resourcetype(R_STONE));
    test_teardown();
}

CuSuite *get_tests_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_resources);
    return suite;
}
