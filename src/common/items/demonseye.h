/* vi: set ts=2:
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_ITM_DEMONSEYE
#define H_ITM_DEMONSEYE
#ifdef __cplusplus
extern "C" {
#endif

extern struct item_type it_demonseye;

extern void register_demonseye(void);
extern boolean give_igjarjuk(const struct unit *, const struct unit *, const struct item_type *, int, struct order *);

#ifdef __cplusplus
}
#endif
#endif
