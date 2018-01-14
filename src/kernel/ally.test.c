#include <platform.h>
#include "types.h"
#include "ally.h"

#include <CuTest.h>
#include <tests.h>

static void test_ally(CuTest * tc)
{
    ally * al = 0;
    struct faction * f1 = test_create_faction(NULL);

    ally_add(&al, f1);
    CuAssertPtrNotNull(tc, al);
    CuAssertPtrEquals(tc, f1, ally_find(al, f1)->faction);

    ally_remove(&al, f1);
    CuAssertPtrEquals(tc, 0, al);
    CuAssertPtrEquals(tc, 0, ally_find(al, f1));
}

static void test_ally_null(CuTest * tc)
{
    ally *a1 = 0, *a2 = 0;

    a1 = ally_add(&a1, 0);
    a2 = ally_add(&a1, 0);
    CuAssertPtrNotNull(tc, a1);
    CuAssertPtrNotNull(tc, a2);
    CuAssertPtrEquals(tc, a2, a1->next);
    CuAssertPtrEquals(tc, 0, a2->next);
    free(a1);
    free(a2);
}

CuSuite *get_ally_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_ally);
    SUITE_ADD_TEST(suite, test_ally_null);
    return suite;
}

