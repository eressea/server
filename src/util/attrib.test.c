#include <platform.h>
#include "attrib.h"

#include <util/gamedata.h>
#include <util/strings.h>

#include <storage.h>
#include <memstream.h>
#include <CuTest.h>
#include <tests.h>
#include <string.h>

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

static void test_attrib_rwstring(CuTest *tc) {
    gamedata data;
    storage store;
    attrib a = { 0 };

    test_setup();
    a.data.v = str_strdup("Hello World");
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    a_writestring(&a, NULL, &store);
    a_finalizestring(&a);
    data.strm.api->rewind(data.strm.handle);
    a_readstring(&a, NULL, &data);
    CuAssertStrEquals(tc, "Hello World", (const char *)a.data.v);
    a_finalizestring(&a);
    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_attrib_rwint(CuTest *tc) {
    gamedata data;
    storage store;
    attrib a = { 0 };

    test_setup();
    a.data.i = 42;
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    a_writeint(&a, NULL, &store);
    a.data.i = 0;
    data.strm.api->rewind(data.strm.handle);
    a_readint(&a, NULL, &data);
    CuAssertIntEquals(tc, 42, a.data.i);
    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_attrib_rwchars(CuTest *tc) {
    gamedata data;
    storage store;
    attrib a = { 0 };

    test_setup();
    a.data.ca[0] = 1;
    a.data.ca[3] = 42;
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    a_writeint(&a, NULL, &store);
    memset(a.data.ca, 0, 4);
    data.strm.api->rewind(data.strm.handle);
    a_readint(&a, NULL, &data);
    CuAssertIntEquals(tc, 1, a.data.ca[0]);
    CuAssertIntEquals(tc, 42, a.data.ca[3]);
    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
}

static void test_attrib_rwshorts(CuTest *tc) {
    gamedata data;
    storage store;
    attrib a = { 0 };
    a.data.sa[0] = -4;
    a.data.sa[1] = 42;

    test_setup();
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    a_writeint(&a, NULL, &store);
    memset(a.data.ca, 0, 4);
    data.strm.api->rewind(data.strm.handle);
    a_readint(&a, NULL, &data);
    CuAssertIntEquals(tc, -4, a.data.sa[0]);
    CuAssertIntEquals(tc, 42, a.data.sa[1]);
    mstream_done(&data.strm);
    gamedata_done(&data);
    test_teardown();
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
    SUITE_ADD_TEST(suite, test_attrib_rwstring);
    SUITE_ADD_TEST(suite, test_attrib_rwint);
    SUITE_ADD_TEST(suite, test_attrib_rwchars);
    SUITE_ADD_TEST(suite, test_attrib_rwshorts);
    return suite;
}
