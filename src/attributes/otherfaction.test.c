#include <platform.h>

#include "otherfaction.h"

#include <kernel/config.h>
#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/faction.h>

#include <util/attrib.h>

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>
#include <assert.h>

static void test_rules(CuTest *tc) {
    test_cleanup();
    set_param(&global.parameters, "stealth.faction.other", NULL);
    CuAssertIntEquals(tc, true, rule_stealth_other());
    set_param(&global.parameters, "stealth.faction.other", "0");
    CuAssertIntEquals(tc, false, rule_stealth_other());
    set_param(&global.parameters, "stealth.faction.other", "1");
    CuAssertIntEquals(tc, true, rule_stealth_other());

    set_param(&global.parameters, "stealth.faction.anon", NULL);
    CuAssertIntEquals(tc, true, rule_stealth_anon());
    set_param(&global.parameters, "stealth.faction.anon", "0");
    CuAssertIntEquals(tc, false, rule_stealth_anon());
    set_param(&global.parameters, "stealth.faction.anon", "1");
    CuAssertIntEquals(tc, true, rule_stealth_anon());
    test_cleanup();
}

static void test_otherfaction(CuTest *tc) {
    unit *u;
    faction *f;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    f = test_create_faction(0);
    set_param(&global.parameters, "stealth.faction.other", "1");
    CuAssertIntEquals(tc, true, rule_stealth_other());
    CuAssertPtrEquals(tc, u->faction, visible_faction(f, u));
    a_add(&u->attribs, make_otherfaction(f));
    CuAssertPtrEquals(tc, f, visible_faction(f, u));
    test_cleanup();
}

CuSuite *get_otherfaction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_rules);
    SUITE_ADD_TEST(suite, test_otherfaction);
    return suite;
}
