/* vi: set ts=2:
 *
 *	$Id: names.h,v 1.1 2001/01/25 09:37:57 enno Exp $
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


char *untoten_name(struct unit * u);
char *skeleton_name(struct unit * u);
char *zombie_name(struct unit * u);
char *ghoul_name(struct unit * u);
char *drachen_name(struct unit *u);
char *dracoid_name(struct unit *u);
const char *shadow_name(const struct unit *u);
const char *abkz(char *s, size_t max);
void name_unit(struct unit *u);
