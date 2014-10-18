#include <platform.h>
#include "attrib.h"

#include <CuTest.h>
#include <tests.h>

static void test_attrib_new(CuTest * tc)
{
    attrib_type at_test = { "test" };
    attrib * a;
    CuAssertPtrNotNull(tc, (a = a_new(&at_test)));
    CuAssertPtrEquals(tc, 0, a->next);
    CuAssertPtrEquals(tc, 0, a->nexttype);
    CuAssertPtrEquals(tc, (void *)a->type, (void *)&at_test);
}


static void test_attrib_add(CuTest * tc)
{
    attrib_type at_test = { "test" };
    attrib *a, *alist = 0;
    CuAssertPtrNotNull(tc, (a = a_new(&at_test)));
    CuAssertPtrEquals(tc, a, a_add(&alist, a));
    CuAssertPtrEquals(tc, a, alist);

    CuAssertPtrNotNull(tc, (a = a_add(&alist, a_new(&at_test))));
    CuAssertPtrEquals(tc, alist->next, a);
    CuAssertPtrEquals(tc, alist->nexttype, a);
}

CuSuite *get_attrib_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_attrib_new);
    return suite;
}
