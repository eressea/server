/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#define MAXDEPTH 1024

extern int search[MAXDEPTH][2];
extern int search_len;

#define NEW_PATH
#ifdef NEW_PATH
extern struct region ** path_find(struct region *start, const struct region *target, int maxlen, boolean (*allowed)(const struct region*, const struct region*));
extern boolean path_exists(struct region *start, const struct region *target, int maxlen, boolean (*allowed)(const struct region*, const struct region*));
extern boolean allowed_swim(const struct region * src, const struct region * target);
extern boolean allowed_fly(const struct region * src, const struct region * target);
extern boolean allowed_walk(const struct region * src, const struct region * target);
#else
extern boolean path_find(struct region *start, struct region *target, int t);
extern boolean path_exists(struct region *start, struct region *target, int t);
extern boolean step(struct region *r, struct region *target, int t, int depth);
#endif

