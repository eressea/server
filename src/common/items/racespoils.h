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

#ifndef H_ITM_SPOILS
#define H_ITM_SPOILS
#ifdef __cplusplus
extern "C" {
#endif

extern struct item_type it_elfspoil;
extern struct item_type it_demonspoil;
extern struct item_type it_goblinspoil;
extern struct item_type it_dwarfspoil;
extern struct item_type it_halflingspoil;
extern struct item_type it_humanspoil;
extern struct item_type it_aquarianspoil;
extern struct item_type it_insectspoil;
extern struct item_type it_catspoil;
extern struct item_type it_orcspoil;
extern struct item_type it_trollspoil;

extern void register_racespoils(void);

#ifdef __cplusplus
}
#endif
#endif
