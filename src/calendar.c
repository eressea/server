#include <platform.h>
#include "calendar.h"
#include <kernel/config.h>

#include <assert.h>
#include <stdlib.h>

int first_month = 0;
int weeks_per_month = 4;
int months_per_year = 12;
char **seasonnames = NULL;
char **weeknames = NULL;
char **weeknames2 = NULL;
char **monthnames = NULL;
int *month_season = NULL;
char *agename = NULL;
int seasons = 0;

int first_turn(void)
{
    return config_get_int("game.start", 1);
}

const gamedate *get_gamedate(int turn, gamedate * gd)
{
    int weeks_per_year = months_per_year * weeks_per_month;
    int t = turn - first_turn();

    assert(gd);
    if (t < 0)
        t = turn;

    gd->week = t % weeks_per_month;       /* 0 - weeks_per_month-1 */
    gd->month = (t / weeks_per_month + first_month) % months_per_year;    /* 0 - months_per_year-1 */
    gd->year = t / (weeks_per_year)+1;
    gd->season = month_season ? month_season[gd->month] : 0;
    return gd;
}

void calendar_cleanup(void)
{
    int i;

    free(agename);

    if (seasonnames) {
        for (i = 0; i != seasons; ++i) {
            free(seasonnames[i]);
        }
        free(seasonnames);
        seasonnames = 0;
    }

    if (monthnames) {
        for (i = 0; i != months_per_year; ++i) {
            free(monthnames[i]);
        }
        free(storms);
        storms = 0;
        free(month_season);
        month_season = 0;
        free(monthnames);
        monthnames = 0;
    }

    for (i = 0; i != weeks_per_month; ++i) {
        if (weeknames)
            free(weeknames[i]);
        if (weeknames2)
            free(weeknames2[i]);
    }
    free(weeknames);
    weeknames = 0;
    free(weeknames2);
    weeknames2 = 0;
}
