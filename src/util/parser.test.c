#include <platform.h>
#include "parser.h"

#include <string.h>
#include <errno.h>
#include <CuTest.h>

static void test_atoip(CuTest *tc) {
    CuAssertIntEquals(tc, 0, atoip("ALLES"));
    CuAssertIntEquals(tc, 0, errno);
    CuAssertIntEquals(tc, 42, atoip("42"));
    CuAssertIntEquals(tc, 0, atoip("-1"));
}

static void test_parse_token(CuTest *tc) {
    char lbuf[8];
    const char *tok;
    const char *str, *orig;

    orig = str = "SHORT TOKEN";
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, (void *)(orig+5), (void *)str);
    CuAssertStrEquals(tc, "SHORT", tok);
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, (void *)(orig + strlen(orig)), (void *)str);
    CuAssertStrEquals(tc, "TOKEN", tok);
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, NULL, (void *)tok);
}

static void test_parse_token_limit(CuTest *tc) {
    char lbuf[8];
    const char *tok;
    const char *str, *orig;

    orig = str = "LONG_TOKEN";
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, (void *)(orig + strlen(orig)), (void *)str);
    CuAssertStrEquals(tc, tok, "LONG_TO");
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, NULL, (void *)tok);
}

static void test_parse_token_limit_utf8(CuTest *tc) {
    char lbuf[8];
    const char *tok;
    const char *orig = "a\xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f"; /* auml ouml uuml szlig, 8 bytes long */
    const char *str = orig+1;

    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, (void *)(orig + strlen(orig)), (void *)str);
    CuAssertStrEquals(tc, tok, "\xc3\xa4\xc3\xb6\xc3\xbc"); /* just three letters fit, 6 bytes long */
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, NULL, (void *)tok);

    str = orig; /* now with an extra byte in the front, maxing out lbuf exactly */
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, (void *)(orig + strlen(orig)), (void *)str);
    CuAssertStrEquals(tc, tok, "a\xc3\xa4\xc3\xb6\xc3\xbc");
    tok = parse_token(&str, lbuf, sizeof(lbuf));
    CuAssertPtrEquals(tc, NULL, (void *)tok);
}

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

static void test_gettoken_short(CuTest *tc) {
    char token[3];
    init_tokens_str("HELP ONE TWO THREE");
    CuAssertStrEquals(tc, "HE", gettoken(token, sizeof(token)));
    CuAssertStrEquals(tc, "HE", token);
    CuAssertStrEquals(tc, "ON", gettoken(token, sizeof(token)));
    CuAssertStrEquals(tc, "TW", gettoken(token, sizeof(token)));
    CuAssertStrEquals(tc, "TH", gettoken(token, sizeof(token)));
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
    SUITE_ADD_TEST(suite, test_atoip);
    SUITE_ADD_TEST(suite, test_skip_token);
    SUITE_ADD_TEST(suite, test_parse_token);
    SUITE_ADD_TEST(suite, test_parse_token_limit);
    SUITE_ADD_TEST(suite, test_parse_token_limit_utf8);
    SUITE_ADD_TEST(suite, test_gettoken);
    SUITE_ADD_TEST(suite, test_gettoken_short);
    SUITE_ADD_TEST(suite, test_getintegers);
    SUITE_ADD_TEST(suite, test_getstrtoken);
    return suite;
}
