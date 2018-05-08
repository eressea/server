#ifdef _MSC_VER
#include <platform.h>
#endif
#include "calendar.h"
#include "move.h" /* storms */

#include "kernel/config.h"
#include "util/log.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int turn = 0;
int first_month = 0;
int weeks_per_month = 3;
int months_per_year = 12;
const char *seasonnames[CALENDAR_SEASONS] = { "winter", "spring", "summer", "fall" };
char **weeknames = NULL;
char **weeknames2 = NULL;
int *month_season = NULL;

const char *calendar_month(int index)
{
    static char result[20];
    snprintf(result, sizeof(result), "month_%d", index + 1);
    return result;
}

const char *calendar_era(void)
{
    static char result[20];
    int era = config_get_int("game.era", 1);
    snprintf(result, sizeof(result), "age_%d", era);
    return result;
}

int first_turn(void)
{
    return config_get_int("game.start", 0);
}

const gamedate *get_gamedate(int turn_now, gamedate * gd)
{
    int weeks_per_year = months_per_year * weeks_per_month;
    int t = turn_now - first_turn();

    assert(gd);
    if (t<0) {
        log_fatal("current turn %d is before first %d",
                turn_now, first_turn());
    }
    assert(t>=0);

    gd->turn = turn_now;
    gd->week = t % weeks_per_month;       /* 0 - weeks_per_month-1 */
    gd->month = (t / weeks_per_month + first_month) % months_per_year;    /* 0 - months_per_year-1 */
    gd->year = 1 + t / weeks_per_year;
    gd->season = month_season ? month_season[gd->month] : 0;
    return gd;
}

void calendar_cleanup(void)
{
    int i;

    for (i = 0; i != weeks_per_month; ++i) {
        if (weeknames)
            free(weeknames[i]);
        if (weeknames2)
            free(weeknames2[i]);
    }

    free(storms);
    storms = 0;
    free(month_season);
    month_season = 0;
    free(weeknames);
    weeknames = 0;
    free(weeknames2);
    weeknames2 = 0;

    first_month = 0;
    weeks_per_month = 3;
    months_per_year = 12;
}
