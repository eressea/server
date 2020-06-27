#ifndef KRNL_CALENDAR_H
#define KRNL_CALENDAR_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum season_t {
        SEASON_WINTER,
        SEASON_SPRING,
        SEASON_SUMMER,
        SEASON_AUTUMN
    } season_t;
#define CALENDAR_SEASONS 4
    extern const char *seasonnames[CALENDAR_SEASONS];

    extern int months_per_year;
    extern season_t *month_season;
    extern int first_month;
    extern int turn;

    extern char **weeknames;
    extern char **weeknames2;
    extern int weeks_per_month;

    typedef struct gamedate {
        int turn;
        int year;
        season_t season;
        int month;
        int week;
    } gamedate;

    const gamedate *get_gamedate(int turn, gamedate * gd);
    season_t calendar_season(int turn);

    void calendar_cleanup(void);
    const char *calendar_month(int index);
    const char *calendar_era(void);
    int first_turn(void);

#ifdef __cplusplus
}
#endif
#endif
