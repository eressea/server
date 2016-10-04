#include <platform.h>
#include <kernel/config.h>
#include "race.h"
#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>
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
    CuAssertDblEquals(tc, 0.0, rc->magres, 0.0);
    CuAssertDblEquals(tc, 0.0, rc->maxaura, 0.0);
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

CuSuite *get_race_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_race_get);
    SUITE_ADD_TEST(suite, test_rc_name);
    SUITE_ADD_TEST(suite, test_rc_defaults);
    SUITE_ADD_TEST(suite, test_rc_find);
    return suite;
}

