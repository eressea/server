/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef ALP_H
#define ALP_H

struct castorder;
/* ------------------------------------------------------------- */
/* Name:		Alp
 * Stufe:		15
 * Gebiet:  Illaun
 * Wirkung:
 * Erschafft ein kleines Monster (den Alp).  Dieser bewegt sich jede
 * zweite Runde auf eine Zieleinheit zu.  Sobald das Ziel erreicht ist,
 * verwandelt es sich in einen Curse auf die Einheit, welches -2 auf
 * alle Talente bewirkt.
 *
 * Fähigkeiten (factypes.c):
 * Der Alp hat mittlere Tarnung (T7) und exzellente Verteidigung
 * (+20, 99% Magieresistenz, siehe factypes.c)
 *
 * TODO: Der Alp-Curse sollte sich durch besondere Antimagie (Tybied)
 * entfernen lassen.
 *
 * (UNITSPELL | ONETARGET | SEARCHGLOBAL | TESTRESISTANCE)
 */

extern int sp_summon_alp(struct castorder *co);
extern void register_alp(void);

struct unit* alp_target(struct unit *alp);
void alp_findet_opfer(struct unit *alp, struct region *r);
	
	

#endif
