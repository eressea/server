#include <platform.h>

#include "magic.h"

#include <kernel/equipment.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/spell.h>

#include <quicklist.h>

#include <CuTest.h>
#include <tests.h>

void test_equipment(CuTest * tc)
{
    equipment * eq;
    unit * u;
    const item_type * it_horses;
    spell *sp;
    sc_mage * mage;

    test_cleanup();
    test_create_race("human");
    enable_skill(SK_MAGIC, true);
    it_horses = test_create_itemtype("horse");
    CuAssertPtrNotNull(tc, it_horses);
    sp = create_spell("testspell", 0);
    CuAssertPtrNotNull(tc, sp);

    CuAssertPtrEquals(tc, 0, get_equipment("herpderp"));
    eq = create_equipment("herpderp");
    CuAssertPtrEquals(tc, eq, get_equipment("herpderp"));

    equipment_setitem(eq, it_horses, "1");
    equipment_setskill(eq, SK_MAGIC, "5");
    equipment_addspell(eq, sp, 1);

    u = test_create_unit(test_create_faction(0), 0);
    equip_unit_mask(u, eq, EQUIP_ALL);
    CuAssertIntEquals(tc, 1, i_get(u->items, it_horses));
    CuAssertIntEquals(tc, 5, get_level(u, SK_MAGIC));

    mage = get_mage(u);
    CuAssertPtrNotNull(tc, mage);
    CuAssertPtrNotNull(tc, mage->spellbook);
    CuAssertTrue(tc, u_hasspell(u, sp));
}

CuSuite *get_equipment_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_equipment);
    return suite;
}
