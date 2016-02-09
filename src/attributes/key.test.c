#include <platform.h>
#include "key.h"

#include <util/attrib.h>
#include <CuTest.h>

static void test_get_set_keys(CuTest *tc) {
    attrib *a = 0;
    key_set(&a, 42);
    CuAssertTrue(tc, key_get(a, 42));
    key_unset(&a, 42);
    CuAssertTrue(tc, !key_get(a, 42));
}

CuSuite *get_key_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_get_set_keys);
    return suite;
}
