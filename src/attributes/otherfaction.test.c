#include "otherfaction.h"

#include <kernel/attrib.h>
#include <kernel/config.h>
#include <kernel/unit.h>
#include <kernel/faction.h>

#include <CuTest.h>
#include <tests.h>

#include <stdbool.h>         // for true, false
#include <stdlib.h>

static void test_rules(CuTest *tc) {
    test_setup();
    config_set("stealth.faction.other", NULL);
    CuAssertIntEquals(tc, true, rule_stealth_other());
    config_set("stealth.faction.other", "0");
    CuAssertIntEquals(tc, false, rule_stealth_other());
    config_set("stealth.faction.other", "1");
    CuAssertIntEquals(tc, true, rule_stealth_other());

    config_set("stealth.faction.anon", NULL);
    CuAssertIntEquals(tc, true, rule_stealth_anon());
    config_set("stealth.faction.anon", "0");
    CuAssertIntEquals(tc, false, rule_stealth_anon());
    config_set("stealth.faction.anon", "1");
    CuAssertIntEquals(tc, true, rule_stealth_anon());
    test_teardown();
}

static void test_otherfaction(CuTest *tc) {
    unit *u;
    faction *f;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    f = test_create_faction_ex(u->faction->race, u->faction->locale);
    config_set("stealth.faction.other", "1");
    CuAssertIntEquals(tc, true, rule_stealth_other());
    CuAssertPtrEquals(tc, u->faction, visible_faction(f, u, NULL));
    a_add(&u->attribs, make_otherfaction(f));
    CuAssertPtrEquals(tc, f, visible_faction(f, u, get_otherfaction(u)));
    test_teardown();
}

CuSuite *get_otherfaction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_rules);
    SUITE_ADD_TEST(suite, test_otherfaction);
    return suite;
}
