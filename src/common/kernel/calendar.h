#ifndef KRNL_CALENDAR_H
#define KRNL_CALENDAR_H

extern char *agename;
extern int  first_turn;
extern int  first_month;

extern int  seasons;
extern char **seasonnames;

extern int  months_per_year;
extern char **monthnames;
extern int  *month_season;
extern int  *storms; /* in movement.c */

extern char **weeknames;
extern char **weeknames2;
extern int  weeks_per_month;

#endif
