/* vi: set ts=2:
 *
 * $Id: moved.h,v 1.1 2001/04/12 17:21:41 enno Exp $
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

struct attrib;
struct attrib_type;

extern boolean get_moved(attrib ** alist);
extern void set_moved(struct attrib ** alist);
extern void init_moved(void);

extern struct attrib_type at_moved;

