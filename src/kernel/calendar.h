#ifndef KRNL_CALENDAR_H
#define KRNL_CALENDAR_H

#ifdef __cplusplus
extern "C" {
#endif

    enum {
        SEASON_WINTER,
        SEASON_SPRING,
        SEASON_SUMMER,
        SEASON_AUTUMN
    };

    extern char *agename;
    extern int first_turn;
    extern int first_month;

    extern int seasons;
    extern char **seasonnames;

    extern int months_per_year;
    extern char **monthnames;
    extern int *month_season;
    extern int *storms;           /* in movement.c */

    extern char **weeknames;
    extern char **weeknames2;
    extern int weeks_per_month;

    typedef struct gamedate {
        int year;
        int season;
        int month;
        int week;
    } gamedate;

    extern const gamedate *get_gamedate(int turn, gamedate * gd);
    extern void calendar_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif
