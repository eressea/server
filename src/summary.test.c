#include <platform.h>

#include "summary.h"
#include "calendar.h"

#include <CuTest.h>
#include "tests.h"

#include <stdio.h>

static void test_summary(CuTest * tc)
{
    struct summary *sum;
    test_setup();
    sum = make_summary();
    report_summary(sum, sum, true);
    CuAssertIntEquals(tc, 0, remove("parteien.full"));
    free_summary(sum);
    test_cleanup();
}

CuSuite *get_summary_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_summary);
    return suite;
}
