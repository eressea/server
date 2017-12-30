#include <platform.h>
#include "donations.h"

#include <kernel/faction.h>
#include <kernel/region.h>
#include <tests.h>

#include <CuTest.h>

static void test_add_donation(CuTest *tc) {
    faction *f1, *f2;
    region *r;

    test_setup();
    r = test_create_region(0, 0, 0);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    add_donation(f1, f2, 100, r);
    report_donations();
    CuAssertPtrNotNull(tc, test_find_messagetype(r->individual_messages->msgs, "donation"));
    free_donations();
    test_teardown();
}

CuSuite *get_donations_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_add_donation);
    return suite;
}
