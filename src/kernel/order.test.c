#include <config.h>
#include <kernel/config.h>
#include "order.h"

#include <util/parser.h>
#include <util/language.h>

#include <CuTest.h>
#include <stdlib.h>

static void test_create_order(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang = get_or_create_locale("en");

    locale_setstring(lang, "keyword::move", "MOVE");
    ord = create_order(K_MOVE, lang, "NORTH");
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    init_tokens(ord);
    CuAssertStrEquals(tc, "MOVE NORTH", get_command(ord, cmd, sizeof(cmd)));
    CuAssertStrEquals(tc, "MOVE", getstrtoken());
    CuAssertStrEquals(tc, "NORTH", getstrtoken());
    free_order(ord);
}

static void test_parse_order(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang = get_or_create_locale("en");

    locale_setstring(lang, "keyword::move", "MOVE");
    ord = parse_order("MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    init_tokens(ord);
    CuAssertStrEquals(tc, "MOVE NORTH", get_command(ord, cmd, sizeof(cmd)));
    CuAssertStrEquals(tc, "MOVE", getstrtoken());
    CuAssertStrEquals(tc, "NORTH", getstrtoken());
    free_order(ord);
}

static void test_parse_make(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang = get_or_create_locale("en");

    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    init_locale(lang);
    ord = parse_order("M hurrdurr", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MAKE, getkeyword(ord));
    init_tokens(ord);
    CuAssertStrEquals(tc, "MAKE hurrdurr", get_command(ord, cmd, sizeof(cmd)));
    CuAssertStrEquals(tc, "MAKE", getstrtoken());
    CuAssertStrEquals(tc, "hurrdurr", getstrtoken());
    free_order(ord);
}

static void test_parse_make_temp(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang = get_or_create_locale("en");

    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    locale_setstring(lang, "TEMP", "TEMP");
    init_locale(lang);

    ord = parse_order("M T herp", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MAKETEMP, getkeyword(ord));
    init_tokens(ord);
    CuAssertStrEquals(tc, "MAKETEMP herp", get_command(ord, cmd, sizeof(cmd)));
    CuAssertStrEquals(tc, "MAKETEMP", getstrtoken());
    CuAssertStrEquals(tc, "herp", getstrtoken());
    free_order(ord);
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
    CuAssertIntEquals(tc, K_MAKETEMP, getkeyword(ord));
    init_tokens(ord);
    CuAssertStrEquals(tc, "MAKETEMP herp", get_command(ord, cmd, sizeof(cmd)));
    CuAssertStrEquals(tc, "MAKETEMP", getstrtoken());
    CuAssertStrEquals(tc, "herp", getstrtoken());
    free_order(ord);
}

CuSuite *get_order_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_create_order);
    SUITE_ADD_TEST(suite, test_parse_order);
    SUITE_ADD_TEST(suite, test_parse_make);
    SUITE_ADD_TEST(suite, test_parse_make_temp);
    SUITE_ADD_TEST(suite, test_parse_maketemp);
    return suite;
}
