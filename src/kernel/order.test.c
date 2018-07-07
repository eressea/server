#include <platform.h>
#include <kernel/config.h>
#include "order.h"

#include <kernel/skills.h>
#include <kernel/unit.h>

#include <util/parser.h>
#include <util/language.h>

#include <stream.h>
#include <memstream.h>

#include <tests.h>
#include <CuTest.h>
#include <stdlib.h>
#include <string.h>

static void test_create_order(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_setup();
    lang = test_create_locale();

    ord = create_order(K_MOVE, lang, "NORTH");
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertStrEquals(tc, "move NORTH", get_command(ord, lang, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MOVE, init_order_depr(ord));
    CuAssertStrEquals(tc, "NORTH", getstrtoken());
    free_order(ord);
    test_teardown();
}

static void test_parse_order(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_setup();
    lang = test_create_locale();

    ord = parse_order("MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MOVE, ord->command);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertStrEquals(tc, "move NORTH", get_command(ord, lang, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MOVE, init_order_depr(ord));
    CuAssertStrEquals(tc, "NORTH", getstrtoken());
    free_order(ord);

    ord = parse_order("!MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertTrue(tc, ord->id > 0);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertIntEquals(tc, K_MOVE | CMD_QUIET, ord->command);
    free_order(ord);

    ord = parse_order("@MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertTrue(tc, ord->id > 0);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertIntEquals(tc, K_MOVE | CMD_PERSIST, ord->command);
    free_order(ord);

    ord = parse_order("!@MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertTrue(tc, ord->id > 0);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertIntEquals(tc, K_MOVE | CMD_PERSIST | CMD_QUIET, ord->command);
    free_order(ord);

    ord = parse_order("@!MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertTrue(tc, ord->id > 0);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertIntEquals(tc, K_MOVE | CMD_PERSIST | CMD_QUIET, ord->command);
    free_order(ord);

    ord = parse_order("  !@MOVE NORTH", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertTrue(tc, ord->id > 0);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(ord));
    CuAssertIntEquals(tc, K_MOVE | CMD_PERSIST | CMD_QUIET, ord->command);
    free_order(ord);

    test_teardown();
}

static void test_parse_make(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_setup();
    lang = get_or_create_locale("en");

    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    init_locale(lang);
    ord = parse_order("M hurrdurr", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MAKE, getkeyword(ord));
    CuAssertStrEquals(tc, "MAKE hurrdurr", get_command(ord, lang, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MAKE, init_order_depr(ord));
    CuAssertStrEquals(tc, "hurrdurr", getstrtoken());
    free_order(ord);
    test_teardown();
}

static void test_parse_autostudy(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_setup();
    lang = get_or_create_locale("en");
    locale_setstring(lang, mkname("skill", skillnames[SK_ENTERTAINMENT]), "Entertainment");
    locale_setstring(lang, keyword(K_STUDY), "STUDY");
    locale_setstring(lang, keyword(K_AUTOSTUDY), "AUTOSTUDY");
    locale_setstring(lang, parameters[P_AUTO], "AUTO");
    init_locale(lang);

    ord = parse_order("STUDY AUTO Entertainment", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_AUTOSTUDY, getkeyword(ord));
    CuAssertStrEquals(tc, "AUTOSTUDY Entertainment", get_command(ord, lang, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_AUTOSTUDY, init_order(ord, lang));
    CuAssertStrEquals(tc, "Entertainment", getstrtoken());
    free_order(ord);
    test_teardown();
}

static void test_parse_make_temp(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;

    test_setup();
    lang = get_or_create_locale("en");
    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    locale_setstring(lang, parameters[P_TEMP], "TEMP");
    init_locale(lang);

    ord = parse_order("M T herp", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_MAKETEMP, getkeyword(ord));
    CuAssertStrEquals(tc, "MAKETEMP herp", get_command(ord, lang, cmd, sizeof(cmd)));

    CuAssertIntEquals(tc, K_MAKETEMP, init_order(ord, lang));
    CuAssertStrEquals(tc, "herp", getstrtoken());
    free_order(ord);
    test_teardown();
}

static void test_parse_maketemp(CuTest *tc) {
    char cmd[32];
    order *ord;
    struct locale * lang;
    
    test_setup();

    lang = get_or_create_locale("en");
    locale_setstring(lang, keyword(K_MAKE), "MAKE");
    locale_setstring(lang, keyword(K_MAKETEMP), "MAKETEMP");
    locale_setstring(lang, "TEMP", "TEMP");
    init_locale(lang);

    ord = parse_order("MAKET herp", lang);
    CuAssertPtrNotNull(tc, ord);
    CuAssertStrEquals(tc, "MAKETEMP herp", get_command(ord, lang, cmd, sizeof(cmd)));
    CuAssertIntEquals(tc, K_MAKETEMP, getkeyword(ord));
    CuAssertIntEquals(tc, K_MAKETEMP, init_order(ord, lang));
    CuAssertStrEquals(tc, "herp", getstrtoken());
    free_order(ord);
    test_teardown();
}

static void test_init_order(CuTest *tc) {
    order *ord;
    struct locale * lang;

    test_setup();

    lang = get_or_create_locale("en");
    ord = create_order(K_MAKETEMP, lang, "hurr durr");
    CuAssertIntEquals(tc, K_MAKETEMP, init_order(ord, lang));
    CuAssertStrEquals(tc, "hurr", getstrtoken());
    CuAssertStrEquals(tc, "durr", getstrtoken());
    free_order(ord);
    test_teardown();
}

static void test_getstrtoken(CuTest *tc) {
    init_tokens_str("hurr \"durr\" \"\" \'\'");
    CuAssertStrEquals(tc, "hurr", getstrtoken());
    CuAssertStrEquals(tc, "durr", getstrtoken());
    CuAssertStrEquals(tc, "", getstrtoken());
    CuAssertStrEquals(tc, "", getstrtoken());
    CuAssertStrEquals(tc, 0, getstrtoken());
    init_tokens_str(0);
    CuAssertStrEquals(tc, 0, getstrtoken());
}

static void test_skip_token(CuTest *tc) {
    init_tokens_str("hurr \"durr\"");
    skip_token();
    CuAssertStrEquals(tc, "durr", getstrtoken());
    CuAssertStrEquals(tc, 0, getstrtoken());
}

static void test_replace_order(CuTest *tc) {
    order *orders = 0, *orig, *repl;
    struct locale * lang;

    test_setup();
    lang = test_create_locale();
    orig = create_order(K_MAKE, lang, NULL);
    repl = create_order(K_ALLY, lang, NULL);
    replace_order(&orders, orig, repl);
    CuAssertPtrEquals(tc, 0, orders);
    orders = orig;
    replace_order(&orders, orig, repl);
    CuAssertPtrNotNull(tc, orders);
    CuAssertPtrEquals(tc, 0, orders->next);
    CuAssertIntEquals(tc, getkeyword(repl), getkeyword(orders));
    free_order(orders);
    free_order(repl);
    test_teardown();
}

static void test_get_command(CuTest *tc) {
    struct locale * lang;
    order *ord;
    char buf[64];

    test_setup();
    lang = test_create_locale();
    ord = create_order(K_MAKE, lang, "iron");
    CuAssertStrEquals(tc, "make iron", get_command(ord, lang, buf, sizeof(buf)));
    ord->command |= CMD_QUIET;
    CuAssertStrEquals(tc, "!make iron", get_command(ord, lang, buf, sizeof(buf)));
    ord->command |= CMD_PERSIST;
    CuAssertStrEquals(tc, "!@make iron", get_command(ord, lang, buf, sizeof(buf)));
    ord->command = K_MAKE | CMD_PERSIST;
    CuAssertStrEquals(tc, "@make iron", get_command(ord, lang, buf, sizeof(buf)));
    free_order(ord);
    test_teardown();
}

static void test_is_persistent(CuTest *tc) {
    order *ord;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();

    ord = parse_order("@invalid", lang);
    CuAssertPtrEquals(tc, NULL, ord);
    free_order(ord);

    ord = parse_order("give", lang);
    CuAssertIntEquals(tc, K_GIVE, ord->command);
    CuAssertTrue(tc, !is_persistent(ord));
    free_order(ord);

    ord = parse_order("@give", lang);
    CuAssertTrue(tc, !is_repeated(K_GIVE));
    CuAssertIntEquals(tc, K_GIVE | CMD_PERSIST, ord->command);
    CuAssertTrue(tc, is_persistent(ord));
    free_order(ord);

    ord = parse_order("make", lang);
    CuAssertTrue(tc, is_repeated(K_MAKE));
    CuAssertIntEquals(tc, K_MAKE , ord->command);
    CuAssertTrue(tc, is_persistent(ord));
    free_order(ord);

    ord = parse_order("@move", lang);
    CuAssertIntEquals(tc, K_MOVE | CMD_PERSIST, ord->command);
    CuAssertTrue(tc, !is_persistent(ord));
    free_order(ord);

    ord = parse_order("// comment", lang);
    CuAssertTrue(tc, is_persistent(ord));
    CuAssertIntEquals(tc, K_KOMMENTAR, ord->command);
    free_order(ord);

    test_teardown();
}


static void test_is_silent(CuTest *tc) {
    order *ord;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();

    ord = parse_order("make", lang);
    CuAssertIntEquals(tc, K_MAKE, ord->command);
    CuAssertTrue(tc, !is_silent(ord));
    free_order(ord);

    ord = parse_order("!make", lang);
    CuAssertIntEquals(tc, K_MAKE | CMD_QUIET, ord->command);
    CuAssertTrue(tc, is_silent(ord));
    free_order(ord);

    ord = parse_order("@invalid", lang);
    CuAssertPtrEquals(tc, NULL, ord);
    free_order(ord);

    ord = parse_order("// comment", lang);
    CuAssertTrue(tc, is_persistent(ord));
    CuAssertIntEquals(tc, K_KOMMENTAR, ord->command);
    free_order(ord);

    test_teardown();
}

static void test_study_orders(CuTest *tc) {
    order *ord;
    struct locale *lang;
    const char *s;
    char token[16];

    test_setup();
    lang = test_create_locale();

    ord = create_order(K_STUDY, lang, skillname(SK_CROSSBOW, lang));
    CuAssertIntEquals(tc, K_STUDY, getkeyword(ord));
    CuAssertIntEquals(tc, K_STUDY, init_order(ord, lang));
    s = gettoken(token, sizeof(token));
    CuAssertStrEquals(tc, skillname(SK_CROSSBOW, lang), s);
    CuAssertPtrEquals(tc, NULL, (void *)getstrtoken());
    free_order(ord);

    ord = create_order(K_STUDY, lang, skillname(SK_MAGIC, lang));
    CuAssertIntEquals(tc, K_STUDY, getkeyword(ord));
    CuAssertIntEquals(tc, K_STUDY, init_order(ord, lang));
    s = gettoken(token, sizeof(token));
    CuAssertStrEquals(tc, skillname(SK_MAGIC, lang), s);
    CuAssertPtrEquals(tc, NULL, (void *)getstrtoken());
    free_order(ord);

    ord = create_order(K_STUDY, lang, "%s 100", skillname(SK_MAGIC, lang));
    CuAssertIntEquals(tc, K_STUDY, getkeyword(ord));
    CuAssertIntEquals(tc, K_STUDY, init_order(ord, lang));
    s = gettoken(token, sizeof(token));
    CuAssertStrEquals(tc, skillname(SK_MAGIC, lang), s);
    CuAssertIntEquals(tc, 100, getint());
    free_order(ord);

    test_teardown();
}

static void test_study_order(CuTest *tc) {
    char token[32];
    stream out;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, mkname("skill", skillnames[SK_ALCHEMY]), "Alchemie");
    locale_setstring(lang, "keyword::study", "LERNE");
    init_keywords(lang);
    init_skills(lang);
    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));
    u->thisorder = create_order(K_STUDY, lang, "ALCH");
    CuAssertIntEquals(tc, K_STUDY, init_order(u->thisorder, lang));
    CuAssertStrEquals(tc, skillname(SK_ALCHEMY, lang), gettoken(token, sizeof(token)));

    CuAssertStrEquals(tc, "LERNE Alchemie", get_command(u->thisorder, lang, token, sizeof(token)));

    mstream_init(&out);
    stream_order(&out, u->thisorder, lang, true);
    swrite("\n", 1, 1, &out);
    out.api->rewind(out.handle);
    out.api->readln(out.handle, token, sizeof(token));
    CuAssertStrEquals(tc, "LERNE Alchemie", token);
    mstream_done(&out);

    test_teardown();
}

static void test_study_order_unknown(CuTest *tc) {
    char token[32];
    stream out;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "keyword::study", "LERNE");
    init_keywords(lang);
    init_skills(lang);
    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));
    u->thisorder = create_order(K_STUDY, lang, "Schiffsbau");
    CuAssertIntEquals(tc, K_STUDY, init_order(u->thisorder, lang));
    CuAssertStrEquals(tc, "Schiffsbau", gettoken(token, sizeof(token)));

    CuAssertStrEquals(tc, "LERNE Schiffsbau", get_command(u->thisorder, lang, token, sizeof(token)));

    mstream_init(&out);
    stream_order(&out, u->thisorder, lang, true);
    swrite("\n", 1, 1, &out);
    out.api->rewind(out.handle);
    out.api->readln(out.handle, token, sizeof(token));
    CuAssertStrEquals(tc, "LERNE Schiffsbau", token);
    mstream_done(&out);

    test_teardown();
}

static void test_study_order_quoted(CuTest *tc) {
    char token[32];
    stream out;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, mkname("skill", skillnames[SK_WEAPONLESS]), "Waffenloser Kampf");
    locale_setstring(lang, "keyword::study", "LERNE");
    init_keywords(lang);
    init_skills(lang);
    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));
    u->thisorder = create_order(K_STUDY, lang, "Waffenloser~Kampf");
    CuAssertIntEquals(tc, K_STUDY, init_order(u->thisorder, lang));
    CuAssertStrEquals(tc, skillname(SK_WEAPONLESS, lang), gettoken(token, sizeof(token)));

    CuAssertStrEquals(tc, "LERNE 'Waffenloser Kampf'", get_command(u->thisorder, lang, token, sizeof(token)));

    mstream_init(&out);
    stream_order(&out, u->thisorder, lang, true);
    swrite("\n", 1, 1, &out);
    out.api->rewind(out.handle);
    out.api->readln(out.handle, token, sizeof(token));
    CuAssertStrEquals(tc, "LERNE 'Waffenloser Kampf'", token);
    mstream_done(&out);

    test_teardown();
}

static void test_study_order_unknown_tilde(CuTest *tc) {
    char token[32];
    stream out;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "keyword::study", "LERNE");
    init_keywords(lang);
    init_skills(lang);
    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));
    u->thisorder = create_order(K_STUDY, lang, "Waffenloser~Mampf");
    CuAssertIntEquals(tc, K_STUDY, init_order(u->thisorder, lang));
    CuAssertStrEquals(tc, "Waffenloser Mampf", gettoken(token, sizeof(token)));

    CuAssertStrEquals(tc, "LERNE Waffenloser~Mampf", get_command(u->thisorder, lang, token, sizeof(token)));

    mstream_init(&out);
    stream_order(&out, u->thisorder, lang, true);
    swrite("\n", 1, 1, &out);
    out.api->rewind(out.handle);
    out.api->readln(out.handle, token, sizeof(token));
    CuAssertStrEquals(tc, "LERNE Waffenloser~Mampf", token);
    mstream_done(&out);

    test_teardown();
}

static void test_study_order_unknown_quoted(CuTest *tc) {
    char token[32];
    stream out;
    unit *u;
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, "keyword::study", "LERNE");
    init_keywords(lang);
    init_skills(lang);
    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));
    u->thisorder = create_order(K_STUDY, lang, "'Waffenloser Dampf'");
    CuAssertIntEquals(tc, K_STUDY, init_order(u->thisorder, lang));
    CuAssertStrEquals(tc, "Waffenloser Dampf", gettoken(token, sizeof(token)));

    CuAssertStrEquals(tc, "LERNE 'Waffenloser Dampf'", get_command(u->thisorder, lang, token, sizeof(token)));

    mstream_init(&out);
    stream_order(&out, u->thisorder, lang, true);
    swrite("\n", 1, 1, &out);
    out.api->rewind(out.handle);
    out.api->readln(out.handle, token, sizeof(token));
    CuAssertStrEquals(tc, "LERNE 'Waffenloser Dampf'", token);
    mstream_done(&out);

    test_teardown();
}

static void test_create_order_long(CuTest *tc) {
    char buffer[2048];
    order *ord;
    struct locale *lang;
    stream out;
    const char * longstr = "// BESCHREIBEN EINHEIT \"In weiÃ&#131; &#131; &#131; &#131; &#131; &#131; &#131; &#"
        "131;&#131;&#131;&#131;&#131;&#131;&#131;?e GewÃ&#131;&#131;&#131;&#131;&#13"
        "1;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;Ã&#131;&#131;&#131;"
        "&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;Ã&#131;&#131;&#131;&#"
        "131;&#131;&#131;&#131;&#131;&#131;&#131;&#130;Ã&#131;&#131;&#131;&#131;&#13"
        "1;&#131;&#131;&#131;&#131;&#130;Ã&#131;&#131;&#131;&#131;&#131;&#131;&#131;"
        "&#131;&#130;Ã&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#130;Ã&#131;&#131;&"
        "#131;&#131;&#131;&#131;&#130;Ã&#131;&#131;&#131;&#131;&#131;&#130;Ã&#131;&#"
        "131;&#131;&#131;&#130;Ã&#131;&#131;&#131;&#130;Ã&#131;&#131;&#130;Ã&#131;&#"
        "130;Ã&#130;Â¢&#130;Ã&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&"
        "#131;&#131;&#130;Ã&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#1"
        "31;&#130;Ã&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#130;Ã&#13"
        "1;&#131;&#131;&#131;&#131;&#131;&#131;&#131;&#130;Ã&#131;&#131;&#131;&#131;"
        "&#131;&#131;&#131;&#130[...]hB&#65533;&#65533;2&#65533;xa&#65533;Hv$P&#65533;xa&#65533;&#65533;A&#65533;&#65533;&#65533;A&#65533;&#65533;";
    test_setup();
    lang = test_create_locale();
    ord = parse_order(longstr, lang);
    CuAssertIntEquals(tc, 0, ord->command);
    mstream_init(&out);
    stream_order(&out, ord, lang, true);
    out.api->rewind(out.handle);
    out.api->readln(out.handle, buffer, sizeof(buffer));
    CuAssertIntEquals(tc, 1026, strlen(buffer));
    mstream_done(&out);
    free_order(ord);
    test_teardown();
}

static void test_crescape(CuTest *tc) {
    char buffer[16];
    const char *input = "12345678901234567890";

    CuAssertStrEquals(tc, "1234", crescape("1234", buffer, 16));
    CuAssertPtrEquals(tc, (void *)input, (void *)crescape(input, buffer, 16));

    CuAssertStrEquals(tc, "\\\"1234\\\"", crescape("\"1234\"", buffer, 16));
    CuAssertStrEquals(tc, "\\\"1234\\\"", buffer);

    CuAssertStrEquals(tc, "\\\"1234", crescape("\"1234\"", buffer, 8));

    /* unlike in C strings, only " and \ are escaped: */
    CuAssertStrEquals(tc, "\\\"\\\\\n\r\'", crescape("\"\\\n\r\'", buffer, 16));
}

CuSuite *get_order_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_crescape);
    SUITE_ADD_TEST(suite, test_create_order);
    SUITE_ADD_TEST(suite, test_create_order_long);
    SUITE_ADD_TEST(suite, test_study_orders);
    SUITE_ADD_TEST(suite, test_study_order);
    SUITE_ADD_TEST(suite, test_study_order_unknown);
    SUITE_ADD_TEST(suite, test_study_order_unknown_tilde);
    SUITE_ADD_TEST(suite, test_study_order_unknown_quoted);
    SUITE_ADD_TEST(suite, test_study_order_quoted);
    SUITE_ADD_TEST(suite, test_parse_order);
    SUITE_ADD_TEST(suite, test_parse_make);
    SUITE_ADD_TEST(suite, test_parse_autostudy);
    SUITE_ADD_TEST(suite, test_parse_make_temp);
    SUITE_ADD_TEST(suite, test_parse_maketemp);
    SUITE_ADD_TEST(suite, test_init_order);
    SUITE_ADD_TEST(suite, test_replace_order);
    SUITE_ADD_TEST(suite, test_skip_token);
    SUITE_ADD_TEST(suite, test_getstrtoken);
    SUITE_ADD_TEST(suite, test_get_command);
    SUITE_ADD_TEST(suite, test_is_persistent);
    SUITE_ADD_TEST(suite, test_is_silent);
    return suite;
}
