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
    race *rc;
    test_setup();
    rc = get_race(RC_ELF);
    CuAssertPtrEquals(tc, rc, (void *)rc_find("elf"));
    CuAssertPtrEquals(tc, rc, (void *)rc_get_or_create("elf"));
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

