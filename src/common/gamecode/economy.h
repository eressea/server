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

#ifndef ECONOMY_H
#define ECONOMY_H

/* Welchen Teil des Silbers die Bauern fuer Unterhaltung ausgeben (1/20), und
 * wiviel Silber ein Unterhalter pro Talentpunkt bekommt. */

/* Wieviele Silbermuenzen jeweils auf einmal "getaxed" werden. */

#define TAXFRACTION             10

/* Wieviel Silber pro Talentpunkt geklaut wird. */

#define STEALINCOME             50

/* Teil der Bauern, welche Luxusgueter kaufen und verkaufen (1/100). */

#define TRADE_FRACTION          100

extern attrib_type at_reduceproduction;

extern int income(const struct unit * u);

/* Wieviel Fremde eine Partei pro Woche aufnehmen kann */
#define MAXNEWBIES								5

void scramble(void *v1, int n, size_t width);
void economics(void);
void produce(void);
extern int entertainmoney(const struct region * r);

enum { IC_WORK, IC_ENTERTAIN, IC_TAX, IC_TRADE, IC_TRADETAX, IC_STEAL, IC_MAGIC };
void maintain_buildings(boolean crash);
extern void add_spende(struct faction * f1, struct faction * f2, int betrag, struct region * r);
void report_donations(void);
extern void init_economy(void);

#endif
