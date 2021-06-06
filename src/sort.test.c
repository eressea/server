#include "sort.h"

#include "kernel/faction.h"
#include "kernel/unit.h"
#include "kernel/order.h"
#include "kernel/region.h"

#include "util/base36.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/param.h"

#include "tests.h"
#include <CuTest.h>

static void test_sort_after(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(f, r);
    unit_addorder(u1, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_AFTER]), itoa36(u2->no)));
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    restack_units();
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, NULL, u1->next);

    test_teardown();
}

static void test_sort_before(CuTest *tc) {
    unit *u1, *u2;
    faction *f;
    region *r;

    test_setup();
    u1 = test_create_unit(f = test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(f, r);
    unit_addorder(u2, create_order(K_SORT, f->locale, "%s %s",
        LOC(f->locale, parameters[P_BEFORE]), itoa36(u1->no)));
    CuAssertPtrEquals(tc, u1, r->units);
    CuAssertPtrEquals(tc, u2, u1->next);
    CuAssertPtrEquals(tc, NULL, u2->next);

    restack_units();
    CuAssertPtrEquals(tc, u2, r->units);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, NULL, u1->next);

    test_teardown();
}


CuSuite *get_sort_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_sort_after);
    SUITE_ADD_TEST(suite, test_sort_before);
    return suite;
}
