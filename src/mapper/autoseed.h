/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#ifndef _REGIONLIST_H
#define _REGIONLIST_H

struct regionlist;
struct newfaction;

typedef struct newfaction {
	struct newfaction * next;
	const char * email;
	const struct locale * lang;
	const struct race * race;
	int bonus;
	boolean oldregions;
} newfaction;

extern newfaction * newfactions;

extern void autoseed(struct regionlist * rlist);
extern void get_island(struct regionlist ** rlist);

#endif
