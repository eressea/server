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
#define CALENDAR_SEASONS 4
    extern const char *seasonnames[CALENDAR_SEASONS];

    extern int months_per_year;
    extern int *month_season;
    extern int first_month;

    extern char **weeknames;
    extern char **weeknames2;
    extern int weeks_per_month;

    typedef struct gamedate {
        int year;
        int season;
        int month;
        int week;
    } gamedate;

const gamedate *get_gamedate(int turn, gamedate * gd);
void calendar_cleanup(void);
const char *calendar_month(int index);
const char *calendar_era(void);
int first_turn(void);

#ifdef __cplusplus
}
#endif
#endif
