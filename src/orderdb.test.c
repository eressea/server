#include <platform.h>
#include <kernel/config.h>

#include "orderdb.h"

#include <CuTest.h>
#include <tests.h>

#include <string.h>

static void test_orderdb_open_close(CuTest *tc) {
    db_backend choices[] = { DB_MEMORY, DB_NONE };
    db_backend nochoice[] = { DB_SQLITE, DB_NONE };

    CuAssertIntEquals(tc, DB_MEMORY, orderdb_open(choices));
    orderdb_close();

    CuAssertIntEquals(tc, DB_NONE, orderdb_open(nochoice));

    orderdb_close();
}

static void test_odata_save_load(CuTest *tc) {
    db_backend choices[] = { DB_MEMORY, DB_NONE };
    order_data *od;
    int id;
    const char * s = "GIB enno 1 Hodor";

    CuAssertIntEquals(tc, DB_MEMORY, orderdb_open(choices));

    odata_create(&od, strlen(s) + 1, s);
    CuAssertTrue(tc, od->_refcount >= 1);
    id = odata_save(od);
    odata_release(od);
    CuAssertTrue(tc, id != 0);

    od = odata_load(id);
    CuAssertPtrNotNull(tc, od);
    CuAssertTrue(tc, od->_refcount >= 1);
    CuAssertStrEquals(tc, s, od->_str);
    odata_release(od);

    orderdb_close();
}

CuSuite *get_orderdb_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_orderdb_open_close);
    SUITE_ADD_TEST(suite, test_odata_save_load);

    return suite;
}
