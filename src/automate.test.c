#ifdef _MSC_VER
#include <platform.h>
#endif

#include "automate.h"

#include "kernel/faction.h"
#include "kernel/order.h"
#include "kernel/region.h"
#include "kernel/unit.h"

#include "tests.h"

#include <CuTest.h>

static void test_autostudy_init(CuTest *tc) {
    scholar scholars[4];
    unit *u1, *u2, *u3;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    test_create_unit(f, r);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_level(u2, SK_ENTERTAINMENT, 2);
    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    scholars[3].u = NULL;
    CuAssertIntEquals(tc, 3, autostudy_init(scholars, 4, r));
    CuAssertPtrEquals(tc, u2, scholars[0].u);
    CuAssertIntEquals(tc, 2, scholars[0].level);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, scholars[0].sk);
    CuAssertPtrEquals(tc, u1, scholars[1].u);
    CuAssertIntEquals(tc, 0, scholars[1].level);
    CuAssertIntEquals(tc, 0, scholars[1].learn);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, scholars[1].sk);
    CuAssertPtrEquals(tc, u3, scholars[2].u);
    CuAssertIntEquals(tc, 0, scholars[2].level);
    CuAssertIntEquals(tc, 0, scholars[2].learn);
    CuAssertIntEquals(tc, SK_PERCEPTION, scholars[2].sk);
    CuAssertPtrEquals(tc, NULL, scholars[3].u);
    test_teardown();
}

static void test_autostudy_run(CuTest *tc) {
    scholar scholars[4];
    unit *u1, *u2, *u3;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u1, 2);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    set_number(u2, 10);
    set_level(u2, SK_ENTERTAINMENT, 2);
    u3 = test_create_unit(f, r);
    u3->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_PERCEPTION]);
    set_number(u3, 20);
    scholars[3].u = NULL;
    CuAssertIntEquals(tc, 3, autostudy_init(scholars, 4, r));
    autostudy_run(scholars, 3);
    CuAssertIntEquals(tc, 0, scholars[0].learn);
    CuAssertIntEquals(tc, 20, scholars[1].learn);
    CuAssertIntEquals(tc, 30, scholars[2].learn);
}

CuSuite *get_automate_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_autostudy_init);
    SUITE_ADD_TEST(suite, test_autostudy_run);
    return suite;
}
