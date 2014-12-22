#include <platform.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include "alliance.h"
#include <CuTest.h>
#include <tests.h>

#include <assert.h>

typedef struct alliance_fixture {
    struct race * rc;
    struct faction *f1, *f2;
} alliance_fixture;

static void setup_alliance(alliance_fixture *fix) {
    test_cleanup();
    test_create_world();
    fix->rc = test_create_race("human");
    fix->f1 = test_create_faction(fix->rc);
    fix->f2 = test_create_faction(fix->rc);
    assert(fix->rc && fix->f1 && fix->f2);
}

static void test_alliance_make(CuTest *tc) {
    alliance * al;

    test_cleanup();
    assert(!alliances);
    al = makealliance(1, "Hodor");
    CuAssertPtrNotNull(tc, al);
    CuAssertStrEquals(tc, "Hodor", al->name);
    CuAssertIntEquals(tc, 1, al->id);
    CuAssertIntEquals(tc, 0, al->flags);
    CuAssertPtrEquals(tc, 0, al->members);
    CuAssertPtrEquals(tc, 0, al->_leader);
    CuAssertPtrEquals(tc, 0, al->allies);
    CuAssertPtrEquals(tc, al, findalliance(1));
    CuAssertPtrEquals(tc, al, alliances);
    free_alliances();
    CuAssertPtrEquals(tc, 0, findalliance(1));
    CuAssertPtrEquals(tc, 0, alliances);
    test_cleanup();
}

static void test_alliance_join(CuTest *tc) {
    alliance_fixture fix;
    alliance * al;

    setup_alliance(&fix);
    CuAssertPtrEquals(tc, 0, fix.f1->alliance);
    CuAssertPtrEquals(tc, 0, fix.f2->alliance);
    al = makealliance(1, "Hodor");
    setalliance(fix.f1, al);
    CuAssertPtrEquals(tc, fix.f1, alliance_get_leader(al));
    setalliance(fix.f2, al);
    CuAssertPtrEquals(tc, fix.f1, alliance_get_leader(al));
    CuAssertTrue(tc, is_allied(fix.f1, fix.f2));
    test_cleanup();
}

CuSuite *get_alliance_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_alliance_make);
    SUITE_ADD_TEST(suite, test_alliance_join);
    return suite;
}

