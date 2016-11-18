#include <platform.h>
#include "language.h"

#include <CuTest.h>
#include <tests.h>

extern const char *directions[];

static void test_language(CuTest *tc)
{
    const char *str;
    test_setup();
    default_locale = test_create_locale();
    str = directions[1];
    CuAssertStrEquals(tc, str, locale_getstring(default_locale, str));
    test_cleanup();
}

CuSuite *get_language_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_language);
    return suite;
}
