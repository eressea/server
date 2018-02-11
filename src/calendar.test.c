#include <platform.h>

#include "calendar.h"

#include <kernel/config.h>

#include <CuTest.h>
#include "tests.h"

static void test_calendar_config(CuTest * tc)
{
    gamedate gd;

    test_setup();
    get_gamedate(0, &gd);
    CuAssertIntEquals(tc, 0, first_turn());
    config_set_int("game.start", 42);
    CuAssertIntEquals(tc, 42, first_turn());
    test_teardown();
}

static void test_calendar(CuTest * tc)
{
    gamedate gd;

    test_setup();
    get_gamedate(0, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, 0, gd.season);
    CuAssertIntEquals(tc, 0, gd.month);
    CuAssertIntEquals(tc, 0, gd.week);

    get_gamedate(1, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, 0, gd.season);
    CuAssertIntEquals(tc, 0, gd.month);
    CuAssertIntEquals(tc, 1, gd.week);

    get_gamedate(weeks_per_month, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, 0, gd.season);
    CuAssertIntEquals(tc, 1, gd.month);
    CuAssertIntEquals(tc, 0, gd.week);

    get_gamedate(weeks_per_month*months_per_year, &gd);
    CuAssertIntEquals(tc, 2, gd.year);
    CuAssertIntEquals(tc, 0, gd.season);
    CuAssertIntEquals(tc, 0, gd.month);
    CuAssertIntEquals(tc, 0, gd.week);

    config_set_int("game.start", 42);
    get_gamedate(42, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, 0, gd.season);
    CuAssertIntEquals(tc, 0, gd.month);
    CuAssertIntEquals(tc, 0, gd.week);

    first_month = 2;
    get_gamedate(42, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, 0, gd.season);
    CuAssertIntEquals(tc, 2, gd.month);
    CuAssertIntEquals(tc, 0, gd.week);

    test_teardown();
}

static void test_calendar_season(CuTest * tc)
{
    gamedate gd;

    test_setup();
    month_season = calloc(months_per_year, sizeof(int));

    get_gamedate(0, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, 0, gd.season);
    CuAssertIntEquals(tc, 0, gd.month);
    CuAssertIntEquals(tc, 0, gd.week);

    month_season[1] = 1;
    get_gamedate(weeks_per_month + 1, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, 1, gd.season);
    CuAssertIntEquals(tc, 1, gd.month);
    CuAssertIntEquals(tc, 1, gd.week);

    test_teardown();
}

CuSuite *get_calendar_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_calendar_config);
    SUITE_ADD_TEST(suite, test_calendar);
    SUITE_ADD_TEST(suite, test_calendar_season);
    return suite;
}
