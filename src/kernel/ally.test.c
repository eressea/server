#include "ally.h"
#include "faction.h"

#include <CuTest.h>
#include <tests.h>

static void test_allies_clone(CuTest * tc)
{
    struct allies * al = NULL, *ac;
    struct faction * f;

    test_setup();
    f = test_create_faction();
    CuAssertPtrEquals(tc, NULL, allies_clone(NULL));
    
    ally_set(&al, f, HELP_GUARD);
    ac = allies_clone(al);
    CuAssertPtrNotNull(tc, ac);
    CuAssertTrue(tc, al != ac);
    CuAssertIntEquals(tc, HELP_GUARD, ally_get(ac, f));
    CuAssertIntEquals(tc, HELP_GUARD, ally_get(al, f));
    allies_free(al);
    allies_free(ac);
    test_teardown();
}

static void test_allies(CuTest *tc) {
    struct allies * al = NULL;
    struct faction * f;

    test_setup();
    f = test_create_faction();

    CuAssertIntEquals(tc, 0, ally_get(al, f));
    ally_set(&al, f, 42);
    CuAssertIntEquals(tc, 42, ally_get(al, f));
    ally_set(&al, f, 0);
    CuAssertIntEquals(tc, 0, ally_get(al, f));
    CuAssertPtrEquals(tc, NULL, al);
    test_teardown();
}

static void test_allies_set(CuTest *tc) {
    struct faction *f1, *f2;
    struct allies * al = NULL;

    test_setup();
    f1 = test_create_faction();
    f2 = test_create_faction();

    CuAssertPtrEquals(tc, NULL, al);
    ally_set(&al, f1, HELP_ALL);
    CuAssertPtrNotNull(tc, al);
    ally_set(&al, f1, DONT_HELP);
    CuAssertPtrEquals(tc, NULL, f1->allies);
    ally_set(&al, f1, DONT_HELP);
    CuAssertPtrEquals(tc, NULL, al);

    ally_set(&al, f1, HELP_ALL);
    ally_set(&al, f2, DONT_HELP);
    ally_set(&al, f1, DONT_HELP);
    CuAssertPtrEquals(tc, NULL, al);

    test_teardown();
}

static void test_alliedfaction(CuTest *tc) {
    struct faction *f1, *f2;

    test_setup();
    f1 = test_create_faction();
    f2 = test_create_faction();

    CuAssertIntEquals(tc, 0, alliedfaction(f1, f2, HELP_ALL));
    ally_set(&f1->allies, f2, HELP_GIVE);
    CuAssertIntEquals(tc, HELP_GIVE, alliedfaction(f1, f2, HELP_ALL));
    ally_set(&f1->allies, f2, HELP_ALL);
    CuAssertIntEquals(tc, HELP_ALL, alliedfaction(f1, f2, HELP_ALL));
    f1->flags |= FFL_PAUSED;
    CuAssertIntEquals(tc, HELP_GUARD, alliedfaction(f1, f2, HELP_ALL));

    test_teardown();
}

static void test_alliedunit(CuTest *tc) {
    struct faction *f2, *f1;
    struct unit *u;

    test_setup();
    u = test_create_unit(f1 = test_create_faction(), test_create_plain(0, 0));
    f2 = test_create_faction();

    CuAssertIntEquals(tc, 0, alliedunit(u, f2, HELP_ALL));
    ally_set(&f1->allies, f2, HELP_GIVE);
    CuAssertIntEquals(tc, HELP_GIVE, alliedunit(u, f2, HELP_ALL));
    ally_set(&f1->allies, f2, HELP_ALL);
    CuAssertIntEquals(tc, HELP_ALL, alliedunit(u, f2, HELP_ALL));
    f1->flags |= FFL_PAUSED;
    CuAssertIntEquals(tc, HELP_GUARD, alliedunit(u, f2, HELP_ALL));

    test_teardown();
}


CuSuite *get_ally_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_allies);
    SUITE_ADD_TEST(suite, test_allies_clone);
    SUITE_ADD_TEST(suite, test_allies_set);
    SUITE_ADD_TEST(suite, test_alliedfaction);
    SUITE_ADD_TEST(suite, test_alliedunit);
    return suite;
}

