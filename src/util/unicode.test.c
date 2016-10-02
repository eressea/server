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

static void test_unicode_utf8_to_cp437(CuTest *tc)
{
    const char utf8_str[4] = { 0xc3, 0x98, 'l', 0 }; // &Oslash;l
    char ch;
    size_t sz;
    CuAssertIntEquals(tc, 0, unicode_utf8_to_cp437(&ch, utf8_str, &sz));
    CuAssertIntEquals(tc, 2, sz);
    CuAssertIntEquals(tc, '?', ch);
}

CuSuite *get_unicode_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_unicode_tolower);
    SUITE_ADD_TEST(suite, test_unicode_utf8_to_cp437);
    return suite;
}
