#ifdef _MSC_VER
#include <platform.h>
#endif
#include <CuTest.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "strings.h"

static void test_str_escape(CuTest * tc)
{
    char scratch[64];
    CuAssertStrEquals(tc, "12345678901234567890", str_escape("12345678901234567890", scratch, 16));
    CuAssertStrEquals(tc, "123456789\\\"12345", str_escape("123456789\"1234567890", scratch, 16));
    CuAssertStrEquals(tc, "1234567890123456", str_escape("1234567890123456\"890", scratch, 16));
    CuAssertStrEquals(tc, "hello world", str_escape("hello world", scratch, sizeof(scratch)));
    CuAssertStrEquals(tc, "hello \\\"world\\\"", str_escape("hello \"world\"", scratch, sizeof(scratch)));
    CuAssertStrEquals(tc, "\\\"\\\\", str_escape("\"\\", scratch, sizeof(scratch)));
    CuAssertStrEquals(tc, "\\\\", str_escape("\\", scratch, sizeof(scratch)));
}

static void test_str_replace(CuTest * tc)
{
    char result[64];
    str_replace(result, sizeof(result), "Hello $who!", "$who", "World");
    CuAssertStrEquals(tc, "Hello World!", result);
}

static void test_str_hash(CuTest * tc)
{
    CuAssertIntEquals(tc, 0, str_hash(""));
    CuAssertIntEquals(tc, 140703196, str_hash("Hodor"));
}

static void test_str_slprintf(CuTest * tc)
{
    char buffer[32];

    memset(buffer, 0x7f, sizeof(buffer));

    CuAssertTrue(tc, slprintf(buffer, 4, "%s", "herpderp") > 3);
    CuAssertStrEquals(tc, "her", buffer);

    CuAssertIntEquals(tc, 4, (int)str_slprintf(buffer, 8, "%s", "herp"));
    CuAssertStrEquals(tc, "herp", buffer);
    CuAssertIntEquals(tc, 0x7f, buffer[5]);

    CuAssertIntEquals(tc, 8, (int)str_slprintf(buffer, 8, "%s", "herpderp"));
    CuAssertStrEquals(tc, "herpder", buffer);
    CuAssertIntEquals(tc, 0x7f, buffer[8]);
}

CuSuite *get_strings_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_str_hash);
    SUITE_ADD_TEST(suite, test_str_escape);
    SUITE_ADD_TEST(suite, test_str_replace);
    SUITE_ADD_TEST(suite, test_str_slprintf);
    return suite;
}
