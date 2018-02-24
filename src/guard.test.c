#include <platform.h>

#include "guard.h"
#include "laws.h"
#include "monsters.h"

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/terrain.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <limits.h>

#include <CuTest.h>
#include "tests.h"

static void test_is_guarded(CuTest *tc) {
    unit *u1, *u2;
    region *r;
    race *rc;

    test_setup();
    rc = rc_get_or_create("dragon");
    rc->flags |= RCF_UNARMEDGUARD;
    r = test_create_region(0, 0, NULL);
    u1 = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(rc), r);
    CuAssertPtrEquals(tc, 0, is_guarded(r, u1));
    setguard(u2, true);
    CuAssertPtrEquals(tc, u2, is_guarded(r, u1));
    test_teardown();
}

static void test_guard_unskilled(CuTest * tc)
/* TODO: it would be better to test armedmen() */
{
    unit *u, *ug;
    region *r;
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(test_create_faction(NULL), r);
    ug = test_create_unit(test_create_faction(NULL), r);
    i_change(&ug->items, itype, 1);

    setguard(ug, true);
    CuAssertPtrEquals(tc, NULL, is_guarded(r, u));

    set_level(ug, SK_MELEE, 1);
    setguard(ug, true);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u));

    test_teardown();
}

static void test_guard_armed(CuTest * tc)
{
    unit *u, *ug;
    region *r;
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(test_create_faction(NULL), r);
    ug = test_create_unit(test_create_faction(NULL), r);
    i_change(&ug->items, itype, 1);
    set_level(ug, SK_MELEE, 2);
    setguard(ug, true);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u));
    test_teardown();
}

static void test_is_guard(CuTest * tc)
{
    unit *ug;
    region *r;
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    r = test_create_region(0, 0, NULL);
    ug = test_create_unit(test_create_faction(NULL), r);
    i_change(&ug->items, itype, 1);
    setguard(ug, true);
    CuAssertIntEquals(tc, 0, armedmen(ug, false));
    CuAssertTrue(tc, !is_guard(ug));
    set_level(ug, SK_MELEE, 1);
    CuAssertIntEquals(tc, 1, armedmen(ug, false));
    CuAssertTrue(tc, is_guard(ug));
    test_teardown();
}

static void test_guard_unarmed(CuTest * tc)
{
    unit *u, *ug;
    region *r;
    race *rc;

    test_setup();
    rc = test_create_race("mountainguard");
    rc->flags |= RCF_UNARMEDGUARD;
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(test_create_faction(NULL), r);
    ug = test_create_unit(test_create_faction(rc), r);
    setguard(ug, true);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u));
    test_teardown();
}

static void test_guard_monsters(CuTest * tc)
{
    unit *u, *ug;
    region *r;

    test_setup();
    r = test_create_region(0, 0, NULL);
    u = test_create_unit(test_create_faction(NULL), r);
    ug = test_create_unit(get_monsters(), r);
    setguard(ug, true);
    CuAssertPtrEquals(tc, ug, is_guarded(r, u));
    test_teardown();
}

static void test_update_guard(CuTest * tc)
/* https://bugs.eressea.de/view.php?id=2292 */
{
    unit *ug;
    region *r;
    item_type *itype;
    const struct terrain_type *t_ocean, *t_plain;

    test_setup();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("packice", ARCTIC_REGION);
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    r = test_create_region(0, 0, t_plain);
    ug = test_create_unit(test_create_faction(NULL), r);
    i_change(&ug->items, itype, 1);
    set_level(ug, SK_MELEE, 1);
    setguard(ug, true);
    CuAssertIntEquals(tc, 1, armedmen(ug, false));
    CuAssertTrue(tc, is_guard(ug));

    terraform_region(r, t_ocean);
    update_guards();
    CuAssertTrue(tc, ! is_guard(ug));

    test_teardown();
}

static void test_guard_on(CuTest * tc)
{
    unit *ug;
    region *r;
    item_type *itype;
    terrain_type *t_ocean, *t_plain;

    test_setup();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE);
    r = test_create_region(0, 0, t_plain);
    ug = test_create_unit(test_create_faction(NULL), r);
    i_change(&ug->items, itype, 1);
    set_level(ug, SK_MELEE, 1);
    ug->thisorder = create_order(K_GUARD, ug->faction->locale, NULL);

    setguard(ug, false);
    CuAssertIntEquals(tc, E_GUARD_OK, can_start_guarding(ug));
    guard_on_cmd(ug, ug->thisorder);
    CuAssertTrue(tc, is_guard(ug));

    terraform_region(r, test_create_terrain("packice", ARCTIC_REGION));

    setguard(ug, false);
    CuAssertIntEquals(tc, E_GUARD_OK, can_start_guarding(ug));
    guard_on_cmd(ug, ug->thisorder);
    CuAssertTrue(tc, is_guard(ug));

    terraform_region(r, t_ocean);

    setguard(ug, false);
    CuAssertIntEquals(tc, E_GUARD_TERRAIN, can_start_guarding(ug));
    guard_on_cmd(ug, ug->thisorder);
    CuAssertTrue(tc, !is_guard(ug));
    CuAssertPtrNotNull(tc, test_find_messagetype(ug->faction->msgs, "error2"));

    terraform_region(r, t_plain);

    i_change(&ug->items, itype, -1);
    CuAssertIntEquals(tc, E_GUARD_UNARMED, can_start_guarding(ug));
    guard_on_cmd(ug, ug->thisorder);
    CuAssertTrue(tc, !is_guard(ug));
    CuAssertPtrNotNull(tc, test_find_messagetype(ug->faction->msgs, "unit_unarmed"));
    i_change(&ug->items, itype, 1);

    test_clear_messages(ug->faction);
    set_level(ug, SK_MELEE, 0);
    CuAssertIntEquals(tc, E_GUARD_UNARMED, can_start_guarding(ug));
    guard_on_cmd(ug, ug->thisorder);
    CuAssertTrue(tc, !is_guard(ug));
    CuAssertPtrNotNull(tc, test_find_messagetype(ug->faction->msgs, "unit_unarmed"));
    set_level(ug, SK_MELEE, 2);

    ug->status = ST_FLEE;
    CuAssertIntEquals(tc, E_GUARD_FLEEING, can_start_guarding(ug));
    guard_on_cmd(ug, ug->thisorder);
    CuAssertTrue(tc, !is_guard(ug));
    CuAssertPtrNotNull(tc, test_find_messagetype(ug->faction->msgs, "error320"));
    ug->status = ST_FIGHT;

    config_set("NewbieImmunity", "5");
    CuAssertIntEquals(tc, E_GUARD_NEWBIE, can_start_guarding(ug));
    guard_on_cmd(ug, ug->thisorder);
    CuAssertTrue(tc, !is_guard(ug));
    CuAssertPtrNotNull(tc, test_find_messagetype(ug->faction->msgs, "error304"));
    config_set("NewbieImmunity", NULL);

    test_clear_messages(ug->faction);
    test_teardown();
}

CuSuite *get_guard_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_is_guarded);
    SUITE_ADD_TEST(suite, test_is_guard);
    SUITE_ADD_TEST(suite, test_guard_unskilled);
    SUITE_ADD_TEST(suite, test_guard_on);
    SUITE_ADD_TEST(suite, test_guard_armed);
    SUITE_ADD_TEST(suite, test_guard_unarmed);
    SUITE_ADD_TEST(suite, test_guard_monsters);
    SUITE_ADD_TEST(suite, test_update_guard);
    return suite;
}
