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
    a_remove(&a, a);
    CuAssertPtrEquals(tc, 0, a);
}


static void test_attrib_add(CuTest * tc)
{
    attrib_type at_foo = { "foo" };
    attrib_type at_bar = { "bar" };
    attrib *a, *alist = 0;

    CuAssertPtrNotNull(tc, (a = a_new(&at_foo)));
    CuAssertPtrEquals(tc, a, a_add(&alist, a));
    CuAssertPtrEquals(tc, a, alist);

    CuAssertPtrNotNull(tc, (a = a_add(&alist, a_new(&at_foo))));
    CuAssertPtrEquals_Msg(tc, "new attribute not added after existing", alist->next, a);

    CuAssertPtrNotNull(tc, (a = a_add(&alist, a_new(&at_bar))));
    CuAssertPtrEquals_Msg(tc, "new atribute not added at end of list", alist->next->next, a);

    CuAssertPtrNotNull(tc, (a = a_add(&alist, a_new(&at_foo))));
    CuAssertPtrEquals_Msg(tc, "messages not sorted by type", alist->next->next, a);
    a_removeall(&alist, &at_foo);
    a_removeall(&alist, &at_bar);
}

static void test_attrib_remove_self(CuTest * tc) {
    attrib_type at_foo = { "foo" };
    attrib *a, *alist = 0;

    CuAssertPtrNotNull(tc, a_add(&alist, a_new(&at_foo)));
    CuAssertPtrNotNull(tc, a = a_add(&alist, a_new(&at_foo)));
    CuAssertPtrEquals(tc, a, alist->next);
    CuAssertPtrEquals(tc, 0, alist->nexttype);
    CuAssertIntEquals(tc, 1, a_remove(&alist, alist));
    CuAssertPtrEquals(tc, a, alist);
    a_removeall(&alist, NULL);
}


static void test_attrib_removeall(CuTest * tc) {
    const attrib_type at_foo = { "foo" };
    const attrib_type at_bar = { "bar" };
    attrib *alist = 0, *a;
    a_add(&alist, a_new(&at_foo));
    a = a_add(&alist, a_new(&at_bar));
    a_add(&alist, a_new(&at_foo));
    a_removeall(&alist, &at_foo);
    CuAssertPtrEquals(tc, a, alist);
    CuAssertPtrEquals(tc, 0, alist->next);
    a_add(&alist, a_new(&at_bar));
    a_add(&alist, a_new(&at_foo));
    a_removeall(&alist, NULL);
    CuAssertPtrEquals(tc, 0, alist);
}

static void test_attrib_remove(CuTest * tc)
{
    attrib_type at_foo = { "foo" };
    attrib *a, *alist = 0;

    CuAssertPtrNotNull(tc, a_add(&alist, a_new(&at_foo)));
    CuAssertPtrNotNull(tc, a = a_add(&alist, a_new(&at_foo)));
    CuAssertIntEquals(tc, 1, a_remove(&alist, a));
    CuAssertPtrNotNull(tc, alist);
    CuAssertIntEquals(tc, 1, a_remove(&alist, alist));
    CuAssertPtrEquals(tc, 0, alist);
}

static void test_attrib_nexttype(CuTest * tc)
{
    attrib_type at_foo = { "foo" };
    attrib_type at_bar = { "bar" };
    attrib *a, *alist = 0;
    CuAssertPtrNotNull(tc, (a = a_new(&at_foo)));
    CuAssertPtrEquals(tc, 0, a->nexttype);
    CuAssertPtrEquals(tc, a, a_add(&alist, a));
    CuAssertPtrEquals(tc, 0, alist->nexttype);

    CuAssertPtrNotNull(tc, a_add(&alist, a_new(&at_foo)));
    CuAssertPtrEquals(tc, 0, alist->nexttype);

    CuAssertPtrNotNull(tc, (a = a_add(&alist, a_new(&at_bar))));
    CuAssertPtrEquals(tc, a, alist->nexttype);
    CuAssertPtrEquals(tc, 0, a->nexttype);

    a_remove(&alist, alist);
    CuAssertPtrEquals(tc, a, alist->nexttype);

    CuAssertPtrNotNull(tc, (a = a_add(&alist, a_new(&at_bar))));
    a_remove(&alist, alist->nexttype);
    CuAssertPtrEquals(tc, a, alist->nexttype);

    a_removeall(&alist, &at_foo);
    a_removeall(&alist, &at_bar);
}

CuSuite *get_attrib_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_attrib_new);
    SUITE_ADD_TEST(suite, test_attrib_add);
    SUITE_ADD_TEST(suite, test_attrib_remove);
    SUITE_ADD_TEST(suite, test_attrib_removeall);
    SUITE_ADD_TEST(suite, test_attrib_remove_self);
    SUITE_ADD_TEST(suite, test_attrib_nexttype);
    return suite;
}
