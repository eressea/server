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
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef H_KRNL_SPY
#define H_KRNL_SPY
#ifdef __cplusplus
extern "C" {
#endif

struct unit;
struct region;
struct strlist;

extern int setwere_cmd(struct unit * u, struct order * ord);
extern int setstealth_cmd(struct unit * u, struct order * ord);
extern int spy_cmd(struct unit * u, struct order * ord);
extern int sabotage_cmd(struct unit * u, struct order * ord);
extern void spy_message(int spy, struct unit *u, struct unit *target);

#define OCEAN_SWIMMER_CHANCE 0.1
#define CANAL_SWIMMER_CHANCE 0.9

#ifdef __cplusplus
}
#endif
#endif
