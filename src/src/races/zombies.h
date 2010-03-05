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

#ifndef H_RACE_ZOMBIES
#define H_RACE_ZOMBIES
#ifdef __cplusplus
extern "C" {
#endif

struct unit;

void age_undead(struct unit *u);
void age_skeleton(struct unit *u);
void age_zombie(struct unit *u);
void age_ghoul(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
