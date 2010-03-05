/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */


#ifndef H_KRNL_NAMES
#define H_KRNL_NAMES
#ifdef __cplusplus
extern "C" {
#endif
extern void register_names(void);
const char *undead_name(const struct unit * u);
const char *skeleton_name(const struct unit * u);
const char *zombie_name(const struct unit * u);
const char *ghoul_name(const struct unit * u);
const char *dragon_name(const struct unit *u);
const char *dracoid_name(const struct unit *u);
const char *generic_name(const struct unit *u);
const char *abkz(const char *s, char * buf, size_t size, size_t maxchars);

#ifdef __cplusplus
}
#endif
#endif
