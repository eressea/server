/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2004
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_ATTRIBUTE_VARIABLE
#define H_ATTRIBUTE_VARIABLE

#ifdef __cplusplus
extern "C" {
#endif

const char *get_variable(struct attrib *a, const char *key);
void set_variable(struct attrib **app, const char *key, const char *value);
void delete_variable(struct attrib **app, const char *key);
void init_variable(void);

#ifdef __cplusplus
}
#endif

#endif

