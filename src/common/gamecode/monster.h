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

#ifndef MONSTER_H
#define MONSTER_H

#define DRAGON_RANGE 6 /* Max. Distanz zum nächsten Drachenziel */

void age_illusion(struct unit *u);

direction_t richest_neighbour(struct region * r, int absolut);
void monsters_kill_peasants(void);
void plan_monsters(void);
void age_unit(struct region * r, struct unit * u);
struct unit *random_unit(const struct region * r);
boolean check_overpopulated(struct unit *u);
void check_split(void);

#endif
