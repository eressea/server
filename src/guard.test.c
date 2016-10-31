#include <platform.h>

#include "guard.h"
#include "laws.h"
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

static void test_is_guarded(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    race *rc;

    test_cleanup();
    rc = rc_get_or_create("dragon");
    rc->flags |= RCF_UNARMEDGUARD;
    r = test_create_region(0, 0, 0);
    u1 = test_create_unit(test_create_faction(0), r);
    u2 = test_create_unit(test_create_faction(rc), r);
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1, GUARD_ALL));
    guard(u2, GUARD_ALL);
    CuAssertPtrEquals(tc, u2, is_guarded(r, u1, GUARD_ALL));
    test_cleanup();
}

static void test_guard_unskilled(CuTest * tc)
// TODO: it would be better to test armedmen()
{
    unit *u, *ug;
    region *r;
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, 0.0, NULL, 0, 0, 0, SK_MELEE, 2);
    r = test_create_region(0, 0, 0);
    u = test_create_unit(test_create_faction(0), r);
    ug = test_create_unit(test_create_faction(0), r);
    i_change(&ug->items, itype, 1);
    set_level(ug, SK_MELEE, 1);
    setguard(ug, true);
    CuAssertPtrEquals(tc, 0, is_guarded(r, u, GUARD_PRODUCE));
    test_cleanup();
}

static void test_guard_armed(CuTest * tc)
{
    unit *u, *ug;
    region *r;
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, 0.0, NULL, 0, 0, 0, SK_MELEE, 2);
    r = test_create_region(0, 0, 0);
    u = test_create_unit(test_create_faction(0), r);
    ug = test_create_unit(test_create_faction(0), r);
    i_change(&ug->items, itype, 1);
    set_level(ug, SK_MELEE, 2);
    setguard(ug, true);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u, GUARD_PRODUCE));
    test_cleanup();
}

static void test_is_guard(CuTest * tc)
{
    unit *ug;
    region *r;
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, 0.0, NULL, 0, 0, 0, SK_MELEE, 2);
    r = test_create_region(0, 0, 0);
    ug = test_create_unit(test_create_faction(0), r);
    i_change(&ug->items, itype, 1);
    set_level(ug, SK_MELEE, 2);
    setguard(ug, true);
    CuAssertIntEquals(tc, 1, armedmen(ug, false));
    CuAssertTrue(tc, is_guard(ug, GUARD_RECRUIT));
    set_level(ug, SK_MELEE, 1);
    CuAssertIntEquals(tc, 0, armedmen(ug, false));
    CuAssertTrue(tc, !is_guard(ug, GUARD_RECRUIT));
    set_level(ug, SK_MELEE, 2);
    CuAssertIntEquals(tc, 1, armedmen(ug, false));
    CuAssertTrue(tc, is_guard(ug, GUARD_RECRUIT));
    test_cleanup();
}

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
    setguard(ug, true);
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
    setguard(ug, true);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u, GUARD_PRODUCE));
    test_cleanup();
}

CuSuite *get_guard_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_is_guarded);
    SUITE_ADD_TEST(suite, test_is_guard);
    SUITE_ADD_TEST(suite, test_guard_unskilled);
    SUITE_ADD_TEST(suite, test_guard_armed);
    SUITE_ADD_TEST(suite, test_guard_unarmed);
    SUITE_ADD_TEST(suite, test_guard_monsters);
    return suite;
}
