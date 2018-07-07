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

static void test_autostudy(CuTest *tc) {
    student students[4];
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
    students[3].u = NULL;
    CuAssertIntEquals(tc, 3, autostudy_init(students, 4, r));
    CuAssertPtrEquals(tc, u2, students[0].u);
    CuAssertIntEquals(tc, 2, students[0].level);
    CuAssertIntEquals(tc, 0, students[0].learn);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, students[0].sk);
    CuAssertPtrEquals(tc, u1, students[1].u);
    CuAssertIntEquals(tc, 0, students[1].level);
    CuAssertIntEquals(tc, 0, students[1].learn);
    CuAssertIntEquals(tc, SK_ENTERTAINMENT, students[1].sk);
    CuAssertPtrEquals(tc, u3, students[2].u);
    CuAssertIntEquals(tc, 0, students[2].level);
    CuAssertIntEquals(tc, 0, students[2].learn);
    CuAssertIntEquals(tc, SK_PERCEPTION, students[2].sk);
    CuAssertPtrEquals(tc, NULL, students[3].u);
    test_teardown();
}

CuSuite *get_automate_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_autostudy);
    return suite;
}
