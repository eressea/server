#include "stealth.h"

#include <kernel/unit.h>
#include <kernel/skill.h>

#include <CuTest.h>
#include <tests.h>

static void test_stealth(CuTest *tc) {
    unit *u;

    test_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    set_level(u, SK_STEALTH, 2);
    CuAssertIntEquals(tc, -1, u_geteffstealth(u));
    CuAssertIntEquals(tc, 2, eff_stealth(u, u->region));
    u_seteffstealth(u, 3);
    CuAssertIntEquals(tc, 2, u_geteffstealth(u));
    CuAssertIntEquals(tc, 2, eff_stealth(u, u->region));
    u_seteffstealth(u, 1);
    CuAssertIntEquals(tc, 1, u_geteffstealth(u));
    CuAssertIntEquals(tc, 1, eff_stealth(u, u->region));
    u_seteffstealth(u, -1);
    CuAssertIntEquals(tc, -1, u_geteffstealth(u));
    CuAssertIntEquals(tc, 2, eff_stealth(u, u->region));
    test_teardown();
}

CuSuite *get_stealth_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_stealth);
    return suite;
}
