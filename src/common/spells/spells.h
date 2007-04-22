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

#ifndef H_SPL_SPELLS
#define H_SPL_SPELLS
#ifdef __cplusplus
extern "C" {
#endif

  extern void register_spells(void);
  extern struct curse * shipcurse_flyingship(struct ship* sh, struct unit * mage, double power, int duration);


  /* für Feuerwände: in movement muß das noch explizit getestet werden.
  * besser wäre eine blcok_type::move() routine, die den effekt
  * der Bewegung auf eine struct unit anwendet.
  */
  extern struct border_type bt_chaosgate;
  extern struct border_type bt_firewall;
  typedef struct wall_data {
    struct unit * mage;
    int force;
    boolean active;
  } wall_data;

#ifdef __cplusplus
}
#endif
#endif
