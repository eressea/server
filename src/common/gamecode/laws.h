/* vi: set ts=2:
 *
 *	$Id: laws.h,v 1.3 2001/02/10 10:40:10 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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

#ifndef LAWS_H
#define LAWS_H

void quit(void);
int getoption(void);
int wanderoff(struct region * r, int p);
void demographics(void);
void instant_orders(void);
void last_orders(void);
void set_passw(void);
void stripunit(struct unit * u);
void mail(void);
void find_address(void);
void bewache_an(void);
void remove_unequipped_guarded(void);
extern void sinkships(void);

/* eressea-specific. put somewhere else, please. */
#include "resolve.h"

extern void * resolve_ship(void * data);

void processorders(void);
extern int count_migrants (const struct faction * f);

#endif
