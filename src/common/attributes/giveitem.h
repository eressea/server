/* vi: set ts=2:
 *
 * $Id: giveitem.h,v 1.2 2001/01/26 16:19:39 enno Exp $
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
#ifndef GIVEITEM_H
#define GIVEITEM_H
struct building;
struct item;

extern struct attrib_type at_giveitem;

extern struct attrib * make_giveitem(struct building * b, struct item * items);
extern void init_giveitem(void);
#endif
