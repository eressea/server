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
#ifndef H_GC_CREPORT
#define H_GC_CREPORT
#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

struct faction_list;
struct seen_region;
struct faction;
extern int report_computer(FILE * F, struct faction * f, struct seen_region ** seen, 
	const struct faction_list * addresses, const time_t report_time);
extern void creport_cleanup(void);
extern void creport_init(void);

extern void report_init(void);
extern void report_cleanup(void);

extern int crwritemap(const char * filename);

#ifdef __cplusplus
}
#endif
#endif

