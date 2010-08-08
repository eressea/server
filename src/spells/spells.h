/* vi: set ts=2:
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_SPL_SPELLS
#define H_SPL_SPELLS
#ifdef __cplusplus
extern "C" {
#endif

  struct ship;
  struct curse;
  struct unit;

  extern void register_spells(void);

  void set_spelldata(struct spell * sp);

#ifdef __cplusplus
}
#endif
#endif
