#include <platform.h>

#include "calendar.h"

#include <kernel/config.h>

#include <CuTest.h>
#include "tests.h"

static void test_calendar(CuTest * tc)
{
    gamedate gd;

    test_setup();
    get_gamedate(0, &gd);
    CuAssertIntEquals(tc, 0, gd.month);
    test_cleanup();
}

CuSuite *get_calendar_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_calendar);
    return suite;
}
