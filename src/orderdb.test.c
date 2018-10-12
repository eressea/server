#include <platform.h>
#include <kernel/config.h>

#include "orderdb.h"

#include <CuTest.h>
#include <tests.h>

#include <string.h>

static void test_orderdb(CuTest *tc) {
    order_data *od = NULL;
    const char * s = "GIB enno 1 Hodor";

    odata_create(&od, strlen(s) + 1, s);
    CuAssertPtrNotNull(tc, od);
    CuAssertStrEquals(tc, s, od->_str);
    CuAssertTrue(tc, od->_refcount >= 1);
    odata_release(od);
}

CuSuite *get_orderdb_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_orderdb);

    return suite;
}
