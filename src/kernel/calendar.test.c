#include "calendar.h"

#include <kernel/config.h>

#include <CuTest.h>
#include "tests.h"

#include <stdlib.h>

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

static void setup_calendar(void) {
    int i;

    months_per_year = 4;
    weeks_per_month = 2;
    free(month_season);
    month_season = calloc(months_per_year, sizeof(season_t));
    for (i = 0; i != 4; ++i) {
        month_season[i] = (season_t)i;
    }
}

static void test_calendar_season(CuTest * tc)
{
    test_setup();
    setup_calendar();

    CuAssertIntEquals(tc, SEASON_WINTER, calendar_season(0));
    CuAssertIntEquals(tc, SEASON_WINTER, calendar_season(1));
    CuAssertIntEquals(tc, SEASON_SPRING, calendar_season(2));
    CuAssertIntEquals(tc, SEASON_SPRING, calendar_season(3));
    CuAssertIntEquals(tc, SEASON_SUMMER, calendar_season(4));
    CuAssertIntEquals(tc, SEASON_SUMMER, calendar_season(5));
    CuAssertIntEquals(tc, SEASON_AUTUMN, calendar_season(6));
    CuAssertIntEquals(tc, SEASON_AUTUMN, calendar_season(7));
    CuAssertIntEquals(tc, SEASON_WINTER, calendar_season(8));

    free(month_season);
    month_season = NULL;
    test_teardown();
}

static void test_gamedate(CuTest * tc)
{
    gamedate gd;

    test_setup();
    setup_calendar();

    get_gamedate(0, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, SEASON_WINTER, gd.season);
    CuAssertIntEquals(tc, 0, gd.month);
    CuAssertIntEquals(tc, 0, gd.week);

    get_gamedate(weeks_per_month + 1, &gd);
    CuAssertIntEquals(tc, 1, gd.year);
    CuAssertIntEquals(tc, SEASON_SPRING, gd.season);
    CuAssertIntEquals(tc, 1, gd.month);
    CuAssertIntEquals(tc, 1, gd.week);

    free(month_season);
    month_season = NULL;
    test_teardown();
}

CuSuite *get_calendar_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_calendar_config);
    SUITE_ADD_TEST(suite, test_calendar);
    SUITE_ADD_TEST(suite, test_calendar_season);
    SUITE_ADD_TEST(suite, test_gamedate);
    return suite;
}
