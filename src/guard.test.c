#include <platform.h>

#include "guard.h"
#include "monster.h"

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <limits.h>

#include <CuTest.h>
#include "tests.h"

static void test_guard_unarmed(CuTest * tc)
{
    unit *u, *ug;
    region *r;
    race *rc;

    test_setup();
    rc = test_create_race("mountainguard");
    rc->flags |= RCF_UNARMEDGUARD;
    r = test_create_region(0, 0, 0);
    u = test_create_unit(test_create_faction(0), r);
    ug = test_create_unit(test_create_faction(rc), r);
    setguard(ug, GUARD_ALL);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u, GUARD_PRODUCE));
    test_cleanup();
}

static void test_guard_monsters(CuTest * tc)
{
    unit *u, *ug;
    region *r;

    test_setup();
    r = test_create_region(0, 0, 0);
    u = test_create_unit(test_create_faction(0), r);
    ug = test_create_unit(get_monsters(), r);
    setguard(ug, GUARD_ALL);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u, GUARD_PRODUCE));
    test_cleanup();
}

CuSuite *get_guard_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_guard_unarmed);
    SUITE_ADD_TEST(suite, test_guard_monsters);
    return suite;
}
