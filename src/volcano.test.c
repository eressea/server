#include <tests.h>
#include "volcano.h"

#include <CuTest.h>

static void test_volcano_update(CuTest *tc) {
    test_cleanup();
    volcano_update();
    test_cleanup();
}

CuSuite *get_volcano_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_volcano_update);
    return suite;
}
