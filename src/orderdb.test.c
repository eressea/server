#include <platform.h>
#include <kernel/config.h>

#include "orderdb.h"

#include <CuTest.h>
#include <tests.h>

static void test_orderdb_open_close(CuTest *tc) {
    db_backend choices[] = { DB_MEMORY, DB_NONE };
    db_backend nochoice[] = { DB_SQLITE, DB_NONE };

    CuAssertIntEquals(tc, DB_MEMORY, orderdb_open(choices));
    orderdb_close();

    CuAssertIntEquals(tc, DB_NONE, orderdb_open(nochoice));

    orderdb_close();
}

CuSuite *get_orderdb_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_orderdb_open_close);

    return suite;
}
