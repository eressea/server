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

static void test_skip_token(CuTest *tc) {
    char token[128];
    init_tokens_str("HELP ONE TWO THREE");
    skip_token();
    CuAssertStrEquals(tc, "ONE", gettoken(token, sizeof(token)));
}

static void test_getintegers(CuTest *tc) {
    init_tokens_str("ii 666 666 -42 -42");
    CuAssertIntEquals(tc, 666, getid());
    CuAssertIntEquals(tc, 666, getint());
    CuAssertIntEquals(tc, 666, getuint());
    CuAssertIntEquals(tc, -42, getint());
    CuAssertIntEquals(tc, 0, getuint());
    CuAssertIntEquals(tc, 0, getint());
}

static void test_getstrtoken(CuTest *tc) {
    init_tokens_str("HELP ONE TWO THREE");
    CuAssertStrEquals(tc, "HELP", getstrtoken());
    CuAssertStrEquals(tc, "ONE", getstrtoken());
    CuAssertStrEquals(tc, "TWO", getstrtoken());
    CuAssertStrEquals(tc, "THREE", getstrtoken());
    CuAssertPtrEquals(tc, NULL, (void *)getstrtoken());
}

CuSuite *get_parser_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_skip_token);
    SUITE_ADD_TEST(suite, test_gettoken);
    SUITE_ADD_TEST(suite, test_getintegers);
    SUITE_ADD_TEST(suite, test_getstrtoken);
    return suite;
}
