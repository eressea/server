/* vi: set ts=2:
 *
 * $Id: goodies.h,v 1.1 2001/01/25 09:37:57 enno Exp $
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

#ifndef GOODIES_H
#define GOODIES_H

extern int *intlist_init(void);
extern int *intlist_add(int *i_p, int i);
extern int *intlist_find(int *i_p, int i);
extern int hashstring(const char* s);
extern char *escape_string(char * str, char replace);
extern char *unescape_string(char * str, char replace);
/* grammar constants: */
#define GR_PLURAL     0x01
/* 0x02-0x08 left unused for individual use */
#define GR_ARTICLE     0x10
#define GR_INDEFINITE_ARTICLE     0x20

#endif /* GOODIES_H */
