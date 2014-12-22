#include <platform.h>
#include "parser.h"

#include <CuTest.h>

static void test_gettoken(CuTest *tc) {
    char token[128];
    init_tokens_str("HELP ONE TWO THREE");
    CuAssertStrEquals(tc, "HELP", gettoken(token, sizeof(token)));
    CuAssertStrEquals(tc, "HELP", token);
    CuAssertStrEquals(tc, "ONE", gettoken(token, sizeof(token)));
    CuAssertStrEquals(tc, "TWO", gettoken(token, sizeof(token)));
    CuAssertStrEquals(tc, "THREE", gettoken(token, sizeof(token)));
    CuAssertPtrEquals(tc, NULL, (void *)gettoken(token, sizeof(token)));
    CuAssertStrEquals(tc, "", token);
}

CuSuite *get_parser_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_gettoken);
    return suite;
}
