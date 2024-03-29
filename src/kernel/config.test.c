#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/order.h>

#include <util/base36.h>
#include <util/keyword.h>    // for K_GIVE, enable_keyword, K_WORK, K_ENTERTAIN
#include <util/language.h>
#include <util/param.h>

#include <iniparser.h>
#include <CuTest.h>
#include <tests.h>

#include <dictionary.h>      // for dictionary_new, dictionary_set, dictionary

#include <stdbool.h>         // for false, true
#include <stdlib.h>

static void test_read_unitid(CuTest *tc) {
    unit *u;
    order *ord;
    struct locale *lang;

    test_setup();
    lang = test_create_locale();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    usetalias(u, atoi36("42"));

    ord = create_order(K_GIVE, lang, "TEMP 42");
    init_order(ord, NULL);
    CuAssertIntEquals(tc, u->no, read_unitid(u->faction, u->region));
    free_order(ord);

    ord = create_order(K_GIVE, lang, "8");
    init_order(ord, NULL);
    CuAssertIntEquals(tc, 8, read_unitid(u->faction, u->region));
    free_order(ord);

    ord = create_order(K_GIVE, lang, "");
    init_order(ord, NULL);
    CuAssertIntEquals(tc, -1, read_unitid(u->faction, u->region));
    free_order(ord);

    ord = create_order(K_GIVE, lang, "TEMP");
    init_order(ord, NULL);
    CuAssertIntEquals(tc, -1, read_unitid(u->faction, u->region));
    free_order(ord);

    /* bug https://bugs.eressea.de/view.php?id=1685 */
    ord = create_order(K_GIVE, lang, "##");
    init_order(ord, NULL);
    CuAssertIntEquals(tc, -1, read_unitid(u->faction, u->region));
    free_order(ord);

    test_teardown();
}

static void test_get_set_param(CuTest * tc)
{
    struct params *par = NULL;
    test_setup();
    CuAssertStrEquals(tc, NULL, params_get(par, "foo"));
    params_set(&par, "foo", "bar");
    params_set(&par, "bar", "foo");
    CuAssertStrEquals(tc, "bar", params_get(par, "foo"));
    CuAssertStrEquals(tc, "foo", params_get(par, "bar"));
    params_set(&par, "bar", "bar");
    CuAssertStrEquals(tc, "bar", params_get(par, "bar"));
    params_set(&par, "bar", NULL);
    CuAssertPtrEquals(tc, NULL, (void *)params_get(par, "bar"));
    params_free(&par);
    test_teardown();
}

static void test_param_int(CuTest * tc)
{
    struct params *par = NULL;
    test_setup();
    CuAssertIntEquals(tc, 13, params_get_int(par, "foo", 13));
    params_set(&par, "foo", "23");
    params_set(&par, "bar", "42");
    CuAssertIntEquals(tc, 23, params_get_int(par, "foo", 0));
    CuAssertIntEquals(tc, 42, params_get_int(par, "bar", 0));
    params_free(&par);
    test_teardown();
}

static void test_param_flt(CuTest * tc)
{
    struct params *par = NULL;
    test_setup();
    CuAssertDblEquals(tc, 13, params_get_flt(par, "foo", 13), 0.01);
    params_set(&par, "foo", "23.0");
    params_set(&par, "bar", "42.0");
    CuAssertDblEquals(tc, 23.0, params_get_flt(par, "foo", 0.0), 0.01);
    CuAssertDblEquals(tc, 42.0, params_get_flt(par, "bar", 0.0), 0.01);
    params_free(&par);
    test_teardown();
}

static void test_forbiddenid(CuTest *tc) {
    CuAssertIntEquals(tc, 0, forbiddenid(1));
    CuAssertIntEquals(tc, 1, forbiddenid(0));
    CuAssertIntEquals(tc, 1, forbiddenid(-1));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("temp")));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("tem")));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("te")));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("t")));
}

static void test_default_order(CuTest *tc) {
    order *ord;
    struct locale * loc;

    test_setup();
    loc = test_create_locale();

    ord = default_order(loc);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_WORK, getkeyword(ord));
    free_order(ord);

    enable_keyword(K_WORK, false);
    ord = default_order(loc);
    CuAssertPtrEquals(tc, NULL, ord);
    free_order(ord);

    config_set("orders.default", "entertain");
    ord = default_order(loc);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(ord));
    free_order(ord);
    test_teardown();
}

static void test_config_cache(CuTest *tc) {
    int key = 0;

    test_setup();
    CuAssertTrue(tc, config_changed(&key));
    config_set("hodor", "0");
    CuAssertTrue(tc, config_changed(&key));
    CuAssertTrue(tc, !config_changed(&key));
    free_config();
    CuAssertTrue(tc, config_changed(&key));
    test_teardown();
}

static void test_rules(CuTest *tc) {
    CuAssertIntEquals(tc, HARVEST_WORK, rule_blessed_harvest());
    config_set("rules.blessed_harvest.flags", "15");
    CuAssertIntEquals(tc, 15, rule_blessed_harvest());

    CuAssertTrue(tc, !rule_region_owners());
    config_set("rules.region_owners", "1");
    CuAssertTrue(tc, rule_region_owners());

    CuAssertTrue(tc, rule_stealth_anon());
    config_set("stealth.faction.anon", "0");
    CuAssertTrue(tc, !rule_stealth_anon());

    CuAssertTrue(tc, rule_stealth_other());
    config_set("stealth.faction.other", "0");
    CuAssertTrue(tc, !rule_stealth_other());

    CuAssertIntEquals(tc, GIVE_DEFAULT, rule_give());
    config_set("rules.give.flags", "15");
    CuAssertIntEquals(tc, 15, rule_give());

    CuAssertIntEquals(tc, 0, rule_alliance_limit());
    config_set("rules.limit.alliance", "1");
    CuAssertIntEquals(tc, 1, rule_alliance_limit());

    CuAssertIntEquals(tc, 0, rule_faction_limit());
    config_set("rules.limit.faction", "1000");
    CuAssertIntEquals(tc, 1000, rule_faction_limit());
}

static void test_config_inifile(CuTest *tc) {
    dictionary *ini;
    test_setup();
    ini = dictionary_new(0);
    dictionary_set(ini, "game", NULL);
    iniparser_set(ini, "game:id", "42");
    iniparser_set(ini, "game:name", "Eressea");
    config_set_from(ini, NULL);
    CuAssertStrEquals(tc, "Eressea", config_get("game.name"));
    CuAssertStrEquals(tc, "Eressea", game_name());
    CuAssertIntEquals(tc, 42, game_id());
    iniparser_freedict(ini);
    test_teardown();
}

static void test_findparam(CuTest *tc) {
    struct locale *en, *de;
    test_setup();
    en = get_or_create_locale("en");
    locale_setstring(en, param_name(P_FACTION, NULL), "FACTION");
    CuAssertIntEquals(tc, NOPARAM, get_param("FACTION", en));
    init_parameters(en);
    CuAssertIntEquals(tc, P_FACTION, get_param("FACTION", en));
    de = get_or_create_locale("de");
    locale_setstring(de, param_name(P_FACTION, NULL), "PARTEI");
    CuAssertIntEquals(tc, NOPARAM, get_param("PARTEI", de));
    init_parameters(de);
    CuAssertIntEquals(tc, P_FACTION, get_param("PARTEI", de));
    CuAssertIntEquals(tc, NOPARAM, get_param("HODOR", de));

    CuAssertIntEquals(tc, NOPARAM, get_param("PARTEI", en));
    CuAssertIntEquals(tc, NOPARAM, findparam_block("HODOR", de, false));
    CuAssertIntEquals(tc, P_FACTION, findparam_block("PARTEI", de, true));
    CuAssertIntEquals(tc, NOPARAM, findparam_block("PARTEI", en, false));
    CuAssertIntEquals(tc, P_FACTION, findparam_block("PARTEI", en, true));
    test_teardown();
}

static void test_game_mailcmd(CuTest *tc) {
    test_setup();
    CuAssertStrEquals(tc, "Eressea", game_name());
    CuAssertStrEquals(tc, "ERESSEA", game_mailcmd());
    config_set("game.name", "Hodor");
    config_set("game.mailcmd", NULL);
    CuAssertStrEquals(tc, "Hodor", game_name());
    CuAssertStrEquals(tc, "HODOR", game_mailcmd());
    config_set("game.mailcmd", "ERESSEA");
    CuAssertStrEquals(tc, "ERESSEA", game_mailcmd());
    test_teardown();
}

CuSuite *get_config_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_findparam);
    SUITE_ADD_TEST(suite, test_config_inifile);
    SUITE_ADD_TEST(suite, test_config_cache);
    SUITE_ADD_TEST(suite, test_get_set_param);
    SUITE_ADD_TEST(suite, test_param_int);
    SUITE_ADD_TEST(suite, test_param_flt);
    SUITE_ADD_TEST(suite, test_forbiddenid);
    SUITE_ADD_TEST(suite, test_read_unitid);
    SUITE_ADD_TEST(suite, test_default_order);
    SUITE_ADD_TEST(suite, test_game_mailcmd);
    SUITE_ADD_TEST(suite, test_rules);
    return suite;
}
