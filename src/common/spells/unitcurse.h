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

int cinfo_unit(const locale *, void *, typ_t, curse *, int);
int cinfo_unit_onlyowner(const locale *, void *, typ_t, curse *, int);

/* altlasten */

int cinfo_auraboost(void *, typ_t, curse *, int);
int cinfo_slave(void *, typ_t, curse *, int);
int cinfo_calm(void *, typ_t, curse *, int);
int cinfo_speed(void *, typ_t, curse *, int);
int cinfo_orc(void *, typ_t, curse *, int);
int cinfo_kaelteschutz(void *, typ_t, curse *, int);
int cinfo_sparkle(void *, typ_t, curse *, int);
int cinfo_strength(void *, typ_t, curse *, int);
int cinfo_allskills(void *, typ_t, curse *, int);
int cinfo_skill(void *, typ_t, curse *, int);
int cinfo_itemcloak(void *, typ_t, curse *, int);
int cinfo_fumble(void *, typ_t, curse *, int);

#endif /* _UCURSE_H */
