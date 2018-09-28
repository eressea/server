#include <platform.h>
#include <kernel/config.h>
#include <kernel/faction.h>

#include "db/driver.h"
#include "orderdb.h"

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <string.h>

static void test_save_load_order(CuTest *tc) {
    order_data *od;
    int id;
    const char * s = "GIB enno 1 Hodor";

    test_setup();

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

    test_teardown();
}

static void test_update_faction(CuTest *tc) {
    faction *f;
    int uid;

    test_setup();
    f = test_create_faction(NULL);
    uid = db_driver_faction_save(f->uid, f->no, 0, f->email, f->_password);
    f->uid = uid;
    uid = db_driver_faction_save(f->uid, f->no, 0, f->email, f->_password);
    CuAssertIntEquals(tc, f->uid, uid);
    test_teardown();
}

CuSuite *get_db_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_save_load_order);
    SUITE_ADD_TEST(suite, test_update_faction);

    return suite;
}
