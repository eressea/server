/* vi: set ts=2:
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

#ifndef _UCURSE_H
#define _UCURSE_H
/* Die sollen ganz sicher nicht an andere module exporiert werden, sondern in 
 * einer register-funktion registriert werden:
int cinfo_unit(const struct locale *, void *, enum typ_t, struct curse *, int);
int cinfo_unit_onlyowner(const struct locale *, void *, enum typ_t, struct curse *, int);
*/
/* altlasten */
/*
int cinfo_auraboost(void *, enum typ_t, struct curse *, int);
int cinfo_slave(void *, enum typ_t, struct curse *, int);
int cinfo_calm(void *, enum typ_t, struct curse *, int);
int cinfo_speed(void *, enum typ_t, struct curse *, int);
int cinfo_orc(void *, enum typ_t, struct curse *, int);
int cinfo_kaelteschutz(void *, enum typ_t, struct curse *, int);
int cinfo_sparkle(void *, enum typ_t, struct curse *, int);
int cinfo_strength(void *, enum typ_t, struct curse *, int);
int cinfo_allskills(void *, enum typ_t, struct curse *, int);
int cinfo_skill(void *, enum typ_t, struct curse *, int);
int cinfo_itemcloak(void *, enum typ_t, struct curse *, int);
int cinfo_fumble(void *, enum typ_t, struct curse *, int);
*/

extern void register_unitcurse(void);

#endif /* _UCURSE_H */
