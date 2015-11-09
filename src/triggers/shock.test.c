#include <platform.h>
#include "shock.h"
#include "../magic.h"

#include <kernel/unit.h>
#include <kernel/faction.h>
#include <util/event.h>

#include <tests.h>
#include <CuTest.h>

static void test_shock(CuTest *tc) {
    unit *u;
    trigger *tt;
    test_cleanup();

    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    create_mage(u, M_GRAY);
    set_level(u, SK_MAGIC, 5);
    set_spellpoints(u, 10);
    u->hp = 10;
    tt = trigger_shock(u);
    tt->type->handle(tt, u);
    CuAssertIntEquals(tc, 2, u->hp);
    CuAssertIntEquals(tc, 2, get_spellpoints(u));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "shock"));
    test_cleanup();
}

static void test_shock_low(CuTest *tc) {
    unit *u;
    trigger *tt;
    test_cleanup();

    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    create_mage(u, M_GRAY);
    set_level(u, SK_MAGIC, 5);
    set_spellpoints(u, 1);
    u->hp = 1;
    tt = trigger_shock(u);
    tt->type->handle(tt, u);
    CuAssertIntEquals(tc, 1, u->hp);
    CuAssertIntEquals(tc, 1, get_spellpoints(u));
    test_cleanup();
}

CuSuite *get_shock_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_shock);
    SUITE_ADD_TEST(suite, test_shock_low);
    return suite;
}
