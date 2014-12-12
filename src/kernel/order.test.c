#include <platform.h>
#include <config.h>
#include <kernel/config.h>
#include "order.h"

#include <util/parser.h>
#include <util/language.h>

#include <tests.h>
#include <CuTest.h>
#include <stdlib.h>
#include <string.h>

static void test_create_order(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_cleanup();
    lang = get_or_create_locale("en");

    locale_setstring(lang, "keyword::move", "MOVE");
    ord = create_order(K_MOVE, lang, "NORTH");
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertStrEquals(tc, "MOVE NORTH", get_command(ord, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MOVE, init_order(ord));
    CuAssertStrEquals(tc, "NORTH", getstrtoken());
    free_order(ord);
    test_cleanup();
}

static void test_parse_order(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;
    
    test_cleanup();
    lang = get_or_create_locale("en");

    locale_setstring(lang, "keyword::move", "MOVE");
    init_keyword(lang, K_MOVE, "MOVE");
    ord = parse_order("MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertStrEquals(tc, "MOVE NORTH", get_command(ord, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MOVE, init_order(ord));
    CuAssertStrEquals(tc, "NORTH", getstrtoken());
    free_order(ord);
    test_cleanup();
}

static void test_parse_make(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_cleanup();
    lang = get_or_create_locale("en");

    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    init_locale(lang);
    ord = parse_order("M hurrdurr", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MAKE, getkeyword(ord));
    CuAssertStrEquals(tc, "MAKE hurrdurr", get_command(ord, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MAKE, init_order(ord));
    CuAssertStrEquals(tc, "hurrdurr", getstrtoken());
    free_order(ord);
    test_cleanup();
}

static void test_parse_make_temp(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_cleanup();
    lang = get_or_create_locale("en");
    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    locale_setstring(lang, "TEMP", "TEMP");
    init_locale(lang);

    ord = parse_order("M T herp", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MAKETEMP, getkeyword(ord));
    CuAssertStrEquals(tc, "MAKETEMP herp", get_command(ord, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MAKETEMP, init_order(ord));
    CuAssertStrEquals(tc, "herp", getstrtoken());
    free_order(ord);
    test_cleanup();
}

static void test_parse_maketemp(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang = get_or_create_locale("en");

    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    locale_setstring(lang, "TEMP", "TEMP");
    init_locale(lang);

    ord = parse_order("MAKET herp", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertStrEquals(tc, "MAKETEMP herp", get_command(ord, cmd, sizeof(cmd)));
    CuAssertIntEquals(tc, K_MAKETEMP, getkeyword(ord));
    CuAssertIntEquals(tc, K_MAKETEMP, init_order(ord));
    CuAssertStrEquals(tc, "herp", getstrtoken());
    free_order(ord);
}

static void test_init_order(CuTest *tc) {
    order *ord;
    struct locale * lang = get_or_create_locale("en");

    ord = create_order(K_MAKETEMP, lang, "hurr durr");
    CuAssertIntEquals(tc, K_MAKETEMP, init_order(ord));
    CuAssertStrEquals(tc, "hurr", getstrtoken());
    CuAssertStrEquals(tc, "durr", getstrtoken());
}

static void test_getstrtoken(CuTest *tc) {
    char *cmd = _strdup("hurr \"durr\" \"\" \'\'");
    init_tokens_str(cmd, cmd);
    CuAssertStrEquals(tc, "hurr", getstrtoken());
    CuAssertStrEquals(tc, "durr", getstrtoken());
    CuAssertStrEquals(tc, "", getstrtoken());
    CuAssertStrEquals(tc, "", getstrtoken());
    CuAssertStrEquals(tc, 0, getstrtoken());
    init_tokens_str(0, 0);
    CuAssertStrEquals(tc, 0, getstrtoken());
}

static void test_skip_token(CuTest *tc) {
    char *cmd = _strdup("hurr \"durr\"");
    init_tokens_str(cmd, cmd);
    skip_token();
    CuAssertStrEquals(tc, "durr", getstrtoken());
    CuAssertStrEquals(tc, 0, getstrtoken());
}

CuSuite *get_order_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_create_order);
    SUITE_ADD_TEST(suite, test_parse_order);
    SUITE_ADD_TEST(suite, test_parse_make);
    SUITE_ADD_TEST(suite, test_parse_make_temp);
    SUITE_ADD_TEST(suite, test_parse_maketemp);
    SUITE_ADD_TEST(suite, test_init_order);
    SUITE_ADD_TEST(suite, test_skip_token);
    SUITE_ADD_TEST(suite, test_getstrtoken);
    return suite;
}
