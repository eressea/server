/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

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
        int bonus;
        int subscription;
        bool oldregions;
        struct alliance *allies;
    } newfaction;

#define ISLANDSIZE 20
#define TURNS_PER_ISLAND 5

    extern int autoseed(newfaction ** players, int nsize, int max_agediff);
    extern newfaction *read_newfactions(const char *filename);
    extern int fix_demand(struct region *r);
    extern const struct terrain_type *random_terrain(const struct terrain_type
        *terrains[], int distribution[], int size);

    extern int seed_adamantium(struct region *r, int base);
    extern int build_island_e3(int x, int y, int numfactions, int minsize);

#ifdef __cplusplus
}
#endif
#endif
