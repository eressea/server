#include "donations.h"

#include <kernel/faction.h>
#include <kernel/region.h>
#include <util/message.h>
#include <tests.h>

#include <CuTest.h>

#include <stddef.h>

static void test_add_donation(CuTest *tc) {
    faction *f1, *f2;
    region *r;

    test_setup();
    mt_create_va(mt_new("donation", NULL), "from:faction", "to:faction", "amount:int", MT_NEW_END);
    r = test_create_plain(0, 0);
    f1 = test_create_faction();
    f2 = test_create_faction();
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
