#include <platform.h>
#include <CuTest.h>
#include "unicode.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void test_unicode_tolower(CuTest * tc)
{
    char buffer[32];
    CuAssertIntEquals(tc, 0, unicode_utf8_tolower(buffer, sizeof(buffer), "HeLlO W0Rld"));
    CuAssertStrEquals(tc, "hello w0rld", buffer);
    memset(buffer, 0, sizeof(buffer));
    buffer[5] = 'X';
    CuAssertIntEquals(tc, ENOMEM, unicode_utf8_tolower(buffer, 5, "HeLlO W0Rld"));
    CuAssertStrEquals(tc, "helloX", buffer);
}

CuSuite *get_unicode_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_unicode_tolower);
    return suite;
}
