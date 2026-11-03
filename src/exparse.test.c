#include "kernel/item.h"
#include "exparse.h"

#include "tests.h"
#include <CuTest.h>

static void test_buildings(CuTest *tc) {
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

CuSuite *get_exparse_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_buildings);

    return suite;
}
