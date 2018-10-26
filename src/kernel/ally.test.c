#include <platform.h>
#include "types.h"
#include "ally.h"

#include <CuTest.h>
#include <tests.h>

static void test_ally(CuTest * tc)
{
    struct ally * al = NULL;
    struct faction * f;

    test_setup();
    f = test_create_faction(NULL);
    ally_set(&al, f, HELP_GUARD);
    CuAssertPtrNotNull(tc, al);
    CuAssertIntEquals(tc, HELP_GUARD, ally_get(al, f));

    ally_set(&al, f, 0);
    CuAssertPtrEquals(tc, NULL, al);
    CuAssertIntEquals(tc, 0, ally_get(al, f));
    allies_free(al);
    test_teardown();
}

static void test_allies_clone(CuTest * tc)
{
    struct ally * al = NULL, *ac;
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

CuSuite *get_ally_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_ally);
    SUITE_ADD_TEST(suite, test_allies_clone);
    return suite;
}

