#include <CuTest.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "goodies.h"

static void test_escape_string(CuTest * tc)
{
    char scratch[64];
    CuAssertStrEquals(tc, "12345678901234567890", escape_string("12345678901234567890", scratch, 16));
    CuAssertStrEquals(tc, "123456789\\\"12345", escape_string("123456789\"1234567890", scratch, 16));
    CuAssertStrEquals(tc, "1234567890123456", escape_string("1234567890123456\"890", scratch, 16));
    CuAssertStrEquals(tc, "hello world", escape_string("hello world", scratch, sizeof(scratch)));
    CuAssertStrEquals(tc, "hello \\\"world\\\"", escape_string("hello \"world\"", scratch, sizeof(scratch)));
    CuAssertStrEquals(tc, "\\\"\\\\", escape_string("\"\\", scratch, sizeof(scratch)));
    CuAssertStrEquals(tc, "\\\\", escape_string("\\", scratch, sizeof(scratch)));
}

CuSuite *get_strings_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_escape_string);
    return suite;
}
