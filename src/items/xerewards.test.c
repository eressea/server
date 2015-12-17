#include <platform.h>

#include "xerewards.h"

#include <kernel/unit.h>
#include <kernel/item.h>
#include <kernel/pool.h>
#include <kernel/region.h>

#include <tests.h>
#include <CuTest.h>

static void test_manacrystal(CuTest *tc) {
    test_cleanup();
    test_create_world();
    test_cleanup();
}

static void test_skillpotion(CuTest *tc) {
    unit *u;
    const struct item_type *itype;
    skill* pSkill = NULL;
    int initialWeeks_Entertainment = 0;
    int initialWeeks_Stamina = 0;
    int initialWeeks_Magic = 0;

    test_cleanup();
    test_create_world();
    u = test_create_unit(test_create_faction(NULL), findregion(0, 0));
    itype = test_create_itemtype("skillpotion");
    change_resource(u, itype->rtype, 2);

    learn_skill(u, SK_ENTERTAINMENT, 1.0);
    pSkill = unit_skill(u, SK_ENTERTAINMENT);
    sk_set(pSkill, 5);
    initialWeeks_Entertainment = pSkill->weeks = 4;

    learn_skill(u, SK_STAMINA, 1.0);
    pSkill = unit_skill(u, SK_STAMINA);
    sk_set(pSkill, 5);
    initialWeeks_Stamina = pSkill->weeks = 4;

    learn_skill(u, SK_MAGIC, 1.0);
    pSkill = unit_skill(u, SK_MAGIC);
    sk_set(pSkill, 5);
    initialWeeks_Magic = pSkill->weeks = 4;

    CuAssertIntEquals(tc, 1, use_skillpotion(u, itype, 1, NULL));

    pSkill = unit_skill(u, SK_ENTERTAINMENT);
    CuAssertIntEquals(tc, initialWeeks_Entertainment - 3, pSkill->weeks);

    pSkill = unit_skill(u, SK_STAMINA);
    CuAssertIntEquals(tc, initialWeeks_Stamina - 3, pSkill->weeks);

    pSkill = unit_skill(u, SK_MAGIC);
    CuAssertIntEquals(tc, initialWeeks_Magic - 3, pSkill->weeks);

    test_cleanup();
}

CuSuite *get_xerewards_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_manacrystal);
    SUITE_ADD_TEST(suite, test_skillpotion);
    return suite;
}

