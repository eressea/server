#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "races.h"

#include "kernel/item.h"
#include "kernel/unit.h"
#include "kernel/race.h"

#include <CuTest.h>

#include <stb_ds.h>

#include <stdbool.h>
#include <stdio.h>

#include "tests.h"

static void test_equip_newgoblin(CuTest *tc) {
    unit *u;
    race *rc;

    test_setup();

    it_get_or_create(rt_get_or_create("roi"));
    rc = test_create_race("goblin");
    u = test_create_unit(test_create_faction_ex(rc, NULL), test_create_plain(0, 0));

    CuAssertIntEquals(tc, 1, u->number);
    equip_newunits(u);
    CuAssertIntEquals(tc, 10, u->number);
    CuAssertIntEquals(tc, 20, unit_max_hp(u));
    CuAssertIntEquals(tc, 20*10, u->hp);

    test_teardown();
}


CuSuite *get_races_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_equip_newgoblin);
    return suite;
}
