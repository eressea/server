#include <platform.h>
#include "key.h"
#include "dict.h"

#include <kernel/attrib.h>
#include <util/base36.h>
#include <CuTest.h>
#include <stdlib.h>

static void test_get_set_keys(CuTest *tc) {
    attrib *a = 0;
    key_set(&a, 42, 1);
    key_set(&a, 43, 2);
    key_set(&a, 44, 3);
    CuAssertIntEquals(tc, 1, key_get(a, 42));
    CuAssertIntEquals(tc, 2, key_get(a, 43));
    CuAssertIntEquals(tc, 3, key_get(a, 44));
    key_unset(&a, 42);
    CuAssertIntEquals(tc, 0, key_get(a, 42));
    CuAssertIntEquals(tc, 2, key_get(a, 43));
    CuAssertIntEquals(tc, 3, key_get(a, 44));
    a_removeall(&a, NULL);
}

static attrib *key_set_orig(attrib **alist, int key) {
    attrib * a = a_add(alist, a_new(&at_key));
    a->data.i = key;
    return a;
}

static void test_upgrade_key(CuTest *tc) {
    attrib *alist = 0;
    key_set_orig(&alist, 40);
    key_set_orig(&alist, 41);
    key_set_orig(&alist, 43);
    key_set_orig(&alist, 42);
    key_set_orig(&alist, 44);
    CuAssertPtrNotNull(tc, alist->type->upgrade);
    alist->type->upgrade(&alist, alist);
    CuAssertIntEquals(tc, 1, key_get(alist, 40));
    CuAssertIntEquals(tc, 1, key_get(alist, 41));
    CuAssertIntEquals(tc, 1, key_get(alist, 42));
    CuAssertIntEquals(tc, 1, key_get(alist, 43));
    CuAssertIntEquals(tc, 1, key_get(alist, 44));
    a_removeall(&alist, NULL);
}

static void test_upgrade_dict(CuTest *tc) {
    attrib *a;
    
    a = a_new(&at_dict);

    dict_set(a, "embassy_muschel", 42);
    CuAssertPtrNotNull(tc, a->type->upgrade);
    a->type->upgrade(&a, a);
    CuAssertIntEquals(tc, 42, key_get(a, atoi36("mupL")));
    a_removeall(&a, NULL);
}

CuSuite *get_key_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_get_set_keys);
    SUITE_ADD_TEST(suite, test_upgrade_key);
    SUITE_ADD_TEST(suite, test_upgrade_dict);
    return suite;
}
