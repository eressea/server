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
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef TELEPORT_H
#define TELEPORT_H

#define TE_CENTER_X	1000
#define TE_CENTER_Y	1000
#define ENNOS_PLANE 1
#if ENNOS_PLANE
#define TP_RADIUS 4
#else
#define RADIUS 75
#define TP_RADIUS 2
#endif
struct region *r_standard_to_astral(const struct region *r);
struct region *r_astral_to_standard(const struct region *);
struct regionlist *all_in_range(struct region *r, int n);
struct regionlist *allinhab_in_range(const struct region *r, int n);
void create_teleport_plane(void);
void set_teleport_plane_regiontypes(void);

#endif
