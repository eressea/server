/* vi: set ts=2:
 *
 *	$Id: names.h,v 1.3 2001/02/02 08:40:46 enno Exp $
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


const char *untoten_name(const struct unit * u);
const char *skeleton_name(const struct unit * u);
const char *zombie_name(const struct unit * u);
const char *ghoul_name(const struct unit * u);
const char *drachen_name(const struct unit *u);
const char *dracoid_name(const struct unit *u);
const char *shadow_name(const struct unit *u);
const char *abkz(const char *s, size_t max);
void name_unit(struct unit *u);
