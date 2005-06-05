#include <config.h>
#include "calendar.h"

int first_turn = 0;
int first_month = 0;
int weeks_per_month = 0;
int months_per_year = 0;
char **seasonnames = NULL;
char **weeknames = NULL;
char **weeknames2 = NULL;
char **monthnames = NULL;
int  *month_season = NULL;
char *agename = NULL;
int  seasons = 0;

gamedate *
get_gamedate(int turn, gamedate * gd)
{
  static gamedate staticdate;
  int weeks_per_year = months_per_year * weeks_per_month;
  int t = turn - first_turn;

  if (gd==NULL) gd = &staticdate;
  if (t<0) t = turn;

  gd->week   = t%weeks_per_month;			/* 0 - weeks_per_month-1 */
  gd->month  = (t/weeks_per_month + first_month)%months_per_year;			/* 0 - months_per_year-1 */
  gd->year   = t/(weeks_per_year) + 1;
  gd->season = month_season[gd->month];
  return gd;
}

