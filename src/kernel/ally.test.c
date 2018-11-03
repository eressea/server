#include <platform.h>
#include "types.h"
#include "ally.h"

#include <CuTest.h>
#include <tests.h>

static void test_allies_clone(CuTest * tc)
{
    struct allies * al = NULL, *ac;
    struct faction * f;

    test_setup();
    f = test_create_faction(NULL);
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
    f = test_create_faction(NULL);

    CuAssertIntEquals(tc, 0, ally_get(al, f));
    ally_set(&al, f, 42);
    CuAssertIntEquals(tc, 42, ally_get(al, f));
    ally_set(&al, f, 0);
    CuAssertIntEquals(tc, 0, ally_get(al, f));
    CuAssertPtrEquals(tc, NULL, al);
    test_teardown();
}

CuSuite *get_ally_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_allies);
    SUITE_ADD_TEST(suite, test_allies_clone);
    return suite;
}

