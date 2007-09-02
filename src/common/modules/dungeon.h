/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#ifndef H_MOD_DUNGEON
#define H_MOD_DUNGEON
#ifdef __cplusplus
extern "C" {
#endif

#ifndef DUNGEON_MODULE
#error "must define DUNGEON_MODULE to use this module"
#endif

struct region;
struct plane;
struct building;
struct dungeon;

extern struct dungeon * dungeonstyles;
extern struct region * make_dungeon(const struct dungeon*);
extern void make_dungeongate(struct region * source, struct region * target, const struct dungeon *);
extern void register_dungeon(void);

#ifdef __cplusplus
}
#endif
#endif
