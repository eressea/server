#include <platform.h>
#include <kernel/config.h>
#include "race.h"
#include "item.h"

#include <tests.h>
#include <CuTest.h>

#include <stdlib.h>
#include <limits.h>
#include <assert.h>

static void test_rc_name(CuTest *tc) {
    struct race *rc;
    test_setup();
    rc = test_create_race("human");
    CuAssertStrEquals(tc, "race::human", rc_name_s(rc, NAME_SINGULAR));
    CuAssertStrEquals(tc, "race::human_p", rc_name_s(rc, NAME_PLURAL));
    CuAssertStrEquals(tc, "race::human_d", rc_name_s(rc, NAME_DEFINITIVE));
    CuAssertStrEquals(tc, "race::human_x", rc_name_s(rc, NAME_CATEGORY));
    test_cleanup();
}

static void test_rc_defaults(CuTest *tc) {
    struct race *rc;
    test_setup();
    rc = rc_get_or_create("human");
    CuAssertStrEquals(tc, "human", rc->_name);
    CuAssertIntEquals(tc, 0, rc_armor_bonus(rc));
    CuAssertIntEquals(tc, 0, rc->magres.sa[0]);
    CuAssertIntEquals(tc, 0, rc->healing);
    CuAssertDblEquals(tc, 0.0, rc_maxaura(rc), 0.0);
    CuAssertDblEquals(tc, 1.0, rc->recruit_multi, 0.0);
    CuAssertDblEquals(tc, 1.0, rc->regaura, 0.0);
    CuAssertDblEquals(tc, 1.0, rc->speed, 0.0);
    CuAssertIntEquals(tc, 0, rc->flags);
    CuAssertIntEquals(tc, 0, rc->recruitcost);
    CuAssertIntEquals(tc, 0, rc->maintenance);
    CuAssertIntEquals(tc, 540, rc->capacity);
    CuAssertIntEquals(tc, 20, rc->income);
    CuAssertIntEquals(tc, 1, rc->hitpoints);
    CuAssertIntEquals(tc, 0, rc->armor);
    CuAssertIntEquals(tc, 0, rc->at_bonus);
    CuAssertIntEquals(tc, 0, rc->df_bonus);
    CuAssertIntEquals(tc, 0, rc->battle_flags);
    CuAssertIntEquals(tc, PERSON_WEIGHT, rc->weight);
    test_cleanup();
}

static void test_rc_find(CuTest *tc) {
    race *rc;
    test_setup();
    rc = test_create_race("hungryhippos");
    CuAssertPtrEquals(tc, rc, (void *)rc_find("hungryhippos"));
    test_cleanup();
}

static void test_race_get(CuTest *tc) {
    int cache = 0;
    const race *rc;
    test_setup();
    CuAssertTrue(tc, rc_changed(&cache));
    CuAssertTrue(tc, !rc_changed(&cache));
    rc = rc_get_or_create("elf");
    CuAssertPtrEquals(tc, (void *)rc, (void *)get_race(RC_ELF));
    CuAssertTrue(tc, rc_changed(&cache));
    CuAssertTrue(tc, !rc_changed(&cache));
    CuAssertPtrEquals(tc, (void *)rc, (void *)rc_find("elf"));
    free_races();
    CuAssertTrue(tc, rc_changed(&cache));
    test_cleanup();
}

static void test_old_race(CuTest *tc)
{
    race * rc1, *rc2;
    test_setup();
    test_create_race("dwarf");
    rc1 = test_create_race("elf");
    rc2 = test_create_race("onkel");
    CuAssertIntEquals(tc, RC_ELF, old_race(rc1));
    CuAssertIntEquals(tc, NORACE, old_race(rc2));
    rc2 = test_create_race("human");
    CuAssertIntEquals(tc, RC_ELF, old_race(rc1));
    CuAssertIntEquals(tc, RC_HUMAN, old_race(rc2));
    test_cleanup();
}

static void test_rc_set_param(CuTest *tc) {
    race *rc;
    test_setup();
    rc = test_create_race("human");
    CuAssertPtrEquals(tc, NULL, rc->options);
    rc_set_param(rc, "recruit_multi", "0.5");
    CuAssertDblEquals(tc, 0.5, rc->recruit_multi, 0.0);
    rc_set_param(rc, "migrants.formula", "1");
    CuAssertIntEquals(tc, RCF_MIGRANTS, rc->flags&RCF_MIGRANTS);
    CuAssertIntEquals(tc, MIGRANTS_LOG10, rc_migrants_formula(rc));
    rc_set_param(rc, "ai.scare", "400");
    CuAssertIntEquals(tc, 400, rc_scare(rc));
    rc_set_param(rc, "hunger.damage", "1d10+12");
    CuAssertStrEquals(tc, "1d10+12", rc_hungerdamage(rc));
    test_cleanup();
}

static void test_rc_can_use(CuTest *tc) {
    race *rc;
    item_type *itype;

    test_setup();
    rc = test_create_race("goblin");
    itype = test_create_itemtype("plate");
    CuAssertTrue(tc, rc_can_use(rc, itype));

    /* default case. all items and races in E2 */
    itype->mask_deny = 0;
    rc->mask_item = 0;
    CuAssertTrue(tc, rc_can_use(rc, itype));

    /* some race is forbidden from using this item. */
    itype->mask_deny = 1;

    /* we are not that race. */
    rc->mask_item = 2;
    CuAssertTrue(tc, rc_can_use(rc, itype));

    /* we are that race */
    rc->mask_item = 1;
    CuAssertTrue(tc, ! rc_can_use(rc, itype));

    /* we are not a special race at all */
    rc->mask_item = 0;
    CuAssertTrue(tc, rc_can_use(rc, itype));

    /* only one race is allowed to use this item */
    itype->mask_deny = 0;
    itype->mask_allow = 1;

    /* we are not that race */
    rc->mask_item = 2;
    CuAssertTrue(tc, ! rc_can_use(rc, itype));
    
    /* we are that race */
    rc->mask_item = 1;
    CuAssertTrue(tc, rc_can_use(rc, itype));
    
    /* we are not special */
    rc->mask_item = 0;
    CuAssertTrue(tc, ! rc_can_use(rc, itype));
    
    test_cleanup();
}

CuSuite *get_race_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_race_get);
    SUITE_ADD_TEST(suite, test_old_race);
    SUITE_ADD_TEST(suite, test_rc_name);
    SUITE_ADD_TEST(suite, test_rc_defaults);
    SUITE_ADD_TEST(suite, test_rc_find);
    SUITE_ADD_TEST(suite, test_rc_set_param);
    SUITE_ADD_TEST(suite, test_rc_can_use);
    return suite;
}

