#include "shock.h"

#include "../magic.h"

#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include "kernel/skill.h"    // for SK_MAGIC
#include "kernel/types.h"    // for M_GWYRRD

#include <util/message.h>

#include <tests.h>
#include <CuTest.h>

#include <stdlib.h>

static void shock_setup(void) {
    mt_create_va(mt_new("shock", NULL),
        "mage:unit", "reason:string", MT_NEW_END);
}

static void test_shock(CuTest *tc) {
    unit *u;
    trigger *tt;

    test_setup();
    shock_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    create_mage(u, M_GWYRRD);
    set_level(u, SK_MAGIC, 5);
    set_spellpoints(u, 10);
    u->hp = 10;
    tt = trigger_shock(u);
    tt->type->handle(tt, u);
    CuAssertIntEquals(tc, 2, u->hp);
    CuAssertIntEquals(tc, 2, get_spellpoints(u));
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "shock"));
    t_free(tt);
    free(tt);
    test_teardown();
}

static void test_shock_low(CuTest *tc) {
    unit *u;
    trigger *tt;

    test_setup();
    shock_setup();
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    create_mage(u, M_GWYRRD);
    set_level(u, SK_MAGIC, 5);
    set_spellpoints(u, 1);
    u->hp = 1;
    tt = trigger_shock(u);
    tt->type->handle(tt, u);
    CuAssertIntEquals(tc, 1, u->hp);
    CuAssertIntEquals(tc, 1, get_spellpoints(u));
    t_free(tt);
    free(tt);
    test_teardown();
}

CuSuite *get_shock_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_shock);
    SUITE_ADD_TEST(suite, test_shock_low);
    return suite;
}
