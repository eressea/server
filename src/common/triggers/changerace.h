/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef CHANGERACE_H
#define CHANGERACE_H
#ifdef __cplusplus
extern "C" {
#endif

/* all types we use are defined here to reduce dependencies */
struct trigger_type;
struct trigger;
struct unit;

extern struct trigger_type tt_changerace;

extern struct trigger * trigger_changerace(struct unit * u, const struct race *urace, const struct race *irace);

#ifdef __cplusplus
}
#endif
#endif
