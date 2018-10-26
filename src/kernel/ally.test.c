#include <platform.h>
#include "types.h"
#include "ally.h"

#include <CuTest.h>
#include <tests.h>

static void test_ally(CuTest * tc)
{
    ally * al = NULL;
    struct faction * f1 = test_create_faction(NULL);

    ally_set(&al, f1, HELP_GUARD);
    CuAssertPtrNotNull(tc, al);
    CuAssertIntEquals(tc, HELP_GUARD, ally_get(al, f1));

    ally_set(&al, f1, 0);
    CuAssertPtrEquals(tc, NULL, al);
    CuAssertIntEquals(tc, 0, ally_get(al, f1));
}

CuSuite *get_ally_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_ally);
    return suite;
}

