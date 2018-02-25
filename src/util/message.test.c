#include <platform.h>
#include "message.h"

#include <CuTest.h>
#include <tests.h>

static void test_mt_new(CuTest *tc)
{
    message_type *mt;
    test_setup();
    mt = mt_new_va("test", "name:string", "number:int", MT_NEW_END);
    mt_register(mt);
    CuAssertPtrNotNull(tc, mt);
    CuAssertStrEquals(tc, "test", mt->name);
    CuAssertIntEquals(tc, 2, mt->nparameters);
    CuAssertPtrNotNull(tc, mt->pnames);
    CuAssertStrEquals(tc, "name", mt->pnames[0]);
    CuAssertStrEquals(tc, "number", mt->pnames[1]);
    CuAssertPtrNotNull(tc, mt->types);
    CuAssertStrEquals(tc, "string", mt->types[0]->name);
    CuAssertStrEquals(tc, "int", mt->types[1]->name);
    test_teardown();
}

CuSuite *get_message_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_mt_new);
    return suite;
}
