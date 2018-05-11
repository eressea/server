#include <platform.h>

#include "magic.h"

#include <kernel/callbacks.h>
#include <kernel/equipment.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/spell.h>

#include <CuTest.h>
#include <tests.h>

static void test_equipment(CuTest * tc)
{
    equipment * eq;
    unit * u;
    const item_type * it_horses;
    spell *sp;
    sc_mage * mage;

    test_setup();
    test_create_race("human");
    enable_skill(SK_MAGIC, true);
    it_horses = test_create_itemtype("horse");
    CuAssertPtrNotNull(tc, it_horses);
    sp = create_spell("testspell");
    CuAssertPtrNotNull(tc, sp);

    CuAssertPtrEquals(tc, 0, get_equipment("herpderp"));
    eq = get_or_create_equipment("herpderp");
    CuAssertPtrEquals(tc, eq, get_equipment("herpderp"));

    equipment_setitem(eq, it_horses, "1");
    equipment_setskill(eq, SK_MAGIC, "5");
    equipment_addspell(eq, sp->sname, 1);

    u = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    equip_unit_set(u, eq, EQUIP_ALL);
    CuAssertIntEquals(tc, 1, i_get(u->items, it_horses));
    CuAssertIntEquals(tc, 5, get_level(u, SK_MAGIC));

    mage = get_mage_depr(u);
    CuAssertPtrNotNull(tc, mage);
    CuAssertPtrNotNull(tc, mage->spellbook);
    CuAssertTrue(tc, u_hasspell(u, sp));
    test_teardown();
}

static void test_get_equipment(CuTest * tc)
{
    equipment * eq;

    test_setup();
    eq = create_equipment("catapultammo123");
    CuAssertPtrNotNull(tc, eq);
    CuAssertStrEquals(tc, "catapultammo123", equipment_name(eq));
    eq = get_equipment("catapultammo123");
    CuAssertPtrNotNull(tc, eq);
    CuAssertStrEquals(tc, "catapultammo123", equipment_name(eq));
    eq = get_equipment("catapult");
    CuAssertPtrEquals(tc, NULL, eq);
    eq = create_equipment("catapult");
    CuAssertPtrNotNull(tc, eq);
    CuAssertPtrEquals(tc, eq, get_equipment("catapult"));
    CuAssertStrEquals(tc, "catapult", equipment_name(eq));
    eq = get_equipment("catapultammo123");
    CuAssertPtrNotNull(tc, eq);
    CuAssertStrEquals(tc, "catapultammo123", equipment_name(eq));
    test_teardown();
}

static bool equip_test(unit *u, const char *name, int mask) {
    if (mask & EQUIP_ITEMS) {
        i_change(&u->items, it_find("horse"), 1);
        return true;
    }
    return false;
}

static void test_equipment_callback(CuTest *tc) {
    unit *u;
    item_type *itype;
    test_setup();
    itype = test_create_horse();
    u = test_create_unit(test_create_faction(NULL), test_create_plain(0, 0));
    CuAssertTrue(tc, !equip_unit_mask(u, "horse", EQUIP_ITEMS));
    CuAssertPtrEquals(tc, NULL, u->items);
    callbacks.equip_unit = equip_test;
    CuAssertTrue(tc, equip_unit(u, "horse"));
    CuAssertIntEquals(tc, 1, i_get(u->items, itype));
    CuAssertTrue(tc, !equip_unit_mask(u, "horse", EQUIP_SPELLS));
    CuAssertIntEquals(tc, 1, i_get(u->items, itype));
    test_teardown();
}

CuSuite *get_equipment_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_equipment);
    SUITE_ADD_TEST(suite, test_get_equipment);
    SUITE_ADD_TEST(suite, test_equipment_callback);
    return suite;
}
