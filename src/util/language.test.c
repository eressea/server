#include <platform.h>
#include <config.h>
#include "language.h"

#include <CuTest.h>
#include <tests.h>

static void test_language(CuTest *tc)
{
    test_setup();

    test_cleanup();
}

CuSuite *get_language_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_language);
    return suite;
}
