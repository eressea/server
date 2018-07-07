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
    unit *u1, *u2;
    faction *f;
    region *r;

    test_setup();
    r = test_create_plain(0, 0);
    f = test_create_faction(NULL);
    u1 = test_create_unit(f, r);
    u1->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    u2 = test_create_unit(f, r);
    u2->thisorder = create_order(K_AUTOSTUDY, f->locale, skillnames[SK_ENTERTAINMENT]);
    CuAssertIntEquals(tc, 2, autostudy_init(students, 4, r));
    test_teardown();
}

CuSuite *get_automate_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_autostudy);
    return suite;
}
