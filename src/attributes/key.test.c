#include <platform.h>
#include "key.h"

#include <util/attrib.h>
#include <CuTest.h>
#include <stdlib.h>

static void test_get_set_keys(CuTest *tc) {
    attrib *a = 0;
    key_set(&a, 42);
    key_set(&a, 43);
    key_set(&a, 44);
    CuAssertTrue(tc, key_get(a, 42));
    CuAssertTrue(tc, key_get(a, 43));
    CuAssertTrue(tc, key_get(a, 44));
    key_unset(&a, 42);
    CuAssertTrue(tc, !key_get(a, 42));
    CuAssertTrue(tc, key_get(a, 43));
    CuAssertTrue(tc, key_get(a, 44));
    a_removeall(&a, NULL);
}

static attrib *key_set_orig(attrib **alist, int key) {
    attrib * a = a_add(alist, a_new(&at_key));
    a->data.i = key;
    return a;
}

static void test_upgrade(CuTest *tc) {
    attrib *alist = 0;
    key_set_orig(&alist, 40);
    key_set_orig(&alist, 41);
    key_set_orig(&alist, 42);
    key_set_orig(&alist, 43);
    key_set_orig(&alist, 44);
    CuAssertPtrNotNull(tc, alist->type->upgrade);
    alist->type->upgrade(&alist, alist);
    CuAssertTrue(tc, key_get(alist, 40));
    CuAssertTrue(tc, key_get(alist, 41));
    CuAssertTrue(tc, key_get(alist, 42));
    CuAssertTrue(tc, key_get(alist, 43));
    CuAssertTrue(tc, key_get(alist, 44));
    a_removeall(&alist, NULL);
}

CuSuite *get_key_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_get_set_keys);
    SUITE_ADD_TEST(suite, test_upgrade);
    return suite;
}
