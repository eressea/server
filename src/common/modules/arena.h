/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef ARENA_H
#define ARENA_H
#ifdef __cplusplus
extern "C" {
#endif

#if ARENA_MODULE == 0
#error "must define ARENA_MODULE to use this module"
#endif
/* exports: */
extern struct plane * arena;

extern void register_arena(void);
#ifdef ARENA_CREATION
extern void create_arena(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
