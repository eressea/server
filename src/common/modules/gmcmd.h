/* vi: set ts=2:
 *
 * $Id: gmcmd.h,v 1.3 2001/01/30 20:26:03 enno Exp $
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
struct unit;

extern void init_gmcmd(void);
/* initialize this module */

extern void gmcommands(void);
/* execute commands */


/*
 * doesn't belong in here:
 */
struct attrib * find_key(struct attrib * attribs, int key);
