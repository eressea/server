#ifndef _REGIONLIST_H
#define _REGIONLIST_H
#ifdef __cplusplus
extern "C" {
#endif

    struct newfaction;

    typedef struct newfaction {
        struct newfaction *next;
        char *email;
        char *password;
        const struct locale *lang;
        const struct race *race;
        bool oldregions;
        struct alliance *allies;
    } newfaction;

#define ISLANDSIZE 20
#define TURNS_PER_ISLAND 5

    extern int autoseed(newfaction ** players, int nsize, int max_agediff);
    extern newfaction *read_newfactions(const char *filename);

    extern int build_island(int x, int y, int minsize, newfaction **players, int numfactions);

#ifdef __cplusplus
}
#endif
#endif
