/* vi: set ts=2:
 *
 * $Id: gm.h,v 1.1 2001/02/17 15:04:06 enno Exp $
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

/* this is an attribute used by the kernel (isallied) */

struct plane;
extern struct attrib_type at_gm;

extern struct attrib * make_gm(const struct plane *pl);
extern void init_gm(void);
