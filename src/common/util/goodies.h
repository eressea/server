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

#ifndef GOODIES_H
#define GOODIES_H
#ifdef __cplusplus
extern "C" {
#endif

extern int *intlist_init(void);
extern int *intlist_add(int *i_p, int i);
extern int *intlist_find(int *i_p, int i);
extern unsigned int hashstring(const char* s);

extern char *space_replace(char * str, char replace);
extern const char *escape_string(const char * str, char * buffer, unsigned int len);
extern boolean locale_check(void);
extern char *fstrncat(char * buffer, const char * str, unsigned int size);
/* fast strncat */
/* grammar constants: */
#define GR_PLURAL     0x01
/* 0x02-0x08 left unused for individual use */
#define GR_ARTICLE     0x10
#define GR_INDEFINITE_ARTICLE     0x20

#ifdef __cplusplus
}
#endif
#endif /* GOODIES_H */
