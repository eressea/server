/* vi: set ts=2:
 *
 * $Id: matmod.h,v 1.1 2001/01/25 09:37:57 enno Exp $
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

typedef int (*mm_fun)(const struct unit * u, const struct resource_type * rtype, int value);

extern struct attrib_type at_matmod;
extern struct attrib * make_matmod(mm_fun function);
