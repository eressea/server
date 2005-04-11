/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
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


struct region_list;
struct newfaction;

typedef struct newfaction {
  struct newfaction * next;
  char * email;
  char * password;
  const struct locale * lang;
  const struct race * race;
  int bonus;
  int subscription;
  boolean oldregions;
  struct alliance * allies;
} newfaction;

extern int autoseed(newfaction ** players, int nsize);
extern newfaction * read_newfactions(const char * filename);
extern void get_island(struct region * root, struct region_list ** rlist);
extern int fix_demand(struct region *r);

#ifdef __cplusplus
}
#endif
#endif
