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

#ifndef H_TRG_TIMEOUT_H
#define H_TRG_TIMEOUT_H
#ifdef __cplusplus
extern "C" {
#endif

struct trigger_type;
struct trigger;

extern struct trigger_type tt_timeout;

extern struct trigger * trigger_timeout(int time, struct trigger * callbacks);

#ifdef __cplusplus
}
#endif
#endif
