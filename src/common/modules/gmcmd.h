/* vi: set ts=2:
 *
 * 
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
struct faction;

extern void init_gmcmd(void);
/* initialize this module */

extern void gmcommands(void);
/* execute commands */

extern struct faction * gm_addquest(const char * email, const char * name, int radius, unsigned int flags);

/*
 * doesn't belong in here:
 */
struct attrib * find_key(struct attrib * attribs, int key);
