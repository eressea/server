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

#ifndef H_MOD_GMCMD
#define H_MOD_GMCMD
#ifdef __cplusplus
extern "C" {
#endif

struct plane;
struct attrib;
struct unit;
struct faction;
struct region;

extern void init_gmcmd(void);
/* initialize this module */

extern void gmcommands(void);
/* execute commands */

extern struct faction * gm_addfaction(const char * email, struct plane * p, struct region * r);
extern struct plane * gm_addplane(int radius, unsigned int flags, const char * name);

/*
 * doesn't belong in here:
 */
struct attrib * find_key(struct attrib * attribs, int key);

#ifdef __cplusplus
}
#endif
#endif
