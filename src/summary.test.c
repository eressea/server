#include "summary.h"

#include <tests.h>
#include <CuTest.h>

#include <stdbool.h>  // for true
#include <stdio.h>

static void test_summary(CuTest * tc)
{
    struct summary *sum;
    test_setup();
    test_create_faction();
    test_create_faction();
    sum = make_summary();
    report_summary(sum, true);
    CuAssertIntEquals(tc, 0, remove("parteien.full"));
    CuAssertIntEquals(tc, 0, remove("datum"));
    CuAssertIntEquals(tc, 0, remove("turn"));
    free_summary(sum);
    test_teardown();
}

CuSuite *get_summary_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_summary);
    return suite;
}
