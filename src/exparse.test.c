#include "exparse.h"

#include "kernel/build.h"
#include "kernel/building.h"
#include "kernel/item.h"
#include "kernel/resources.h"

#include "tests.h"
#include <CuTest.h>

static void test_weapons(CuTest *tc) {
    const resource_type *rtype;
    test_setup();
    CuAssertIntEquals(tc, 0, exparse_readfile("tests/fixtures/greatbow.xml"));
    CuAssertPtrNotNull(tc, rtype = rt_find("greatbow"));
    CuAssertStrEquals(tc, "greatbow", rtype->_name);
    CuAssertPtrNotNull(tc, rtype->wtype);
    CuAssertPtrNotNull(tc, rtype->wtype->modifiers);
    CuAssertIntEquals(tc, 0, rtype->wtype->modifiers[2].value);
    test_teardown();
}

static void test_buildings(CuTest *tc) {
    const building_type *btype;
    test_setup();
    CuAssertIntEquals(tc, 0, exparse_readfile("tests/fixtures/magictower.xml"));
    CuAssertPtrNotNull(tc, btype = bt_find("magictower"));
    CuAssertDblEquals(tc, 1.75, btype->auraregen, 0.1);
    test_teardown();
}

static void test_resources(CuTest *tc) {
    const resource_type *rtype;
    test_setup();
    CuAssertIntEquals(tc, 0, exparse_readfile("tests/fixtures/adamantium.xml"));
    CuAssertPtrNotNull(tc, rtype = rt_find("adamantium"));
    CuAssertStrEquals(tc, "adamantium", rtype->_name);
    CuAssertPtrNotNull(tc, rtype->itype);
    CuAssertIntEquals(tc, 200, rtype->itype->weight);
    CuAssertPtrNotNull(tc, rtype->itype->construction);
    CuAssertIntEquals(tc, 8, rtype->itype->construction[0].minskill);
    CuAssertIntEquals(tc, RMT_END, rtype->modifiers[2].type);
    test_teardown();
}

CuSuite *get_exparse_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_weapons);
    SUITE_ADD_TEST(suite, test_resources);
    SUITE_ADD_TEST(suite, test_buildings);

    return suite;
}
