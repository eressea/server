/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef _UMLAUT_H
#define _UMLAUT_H
#ifdef __cplusplus
extern "C" {
#endif

#define E_TOK_NOMATCH (-1)
#define E_TOK_SUCCESS 0
#define NODEHASHSIZE 7
struct tref;

typedef struct tnode {
	struct tref * next[NODEHASHSIZE];
	unsigned char flags;
	void * id;
} tnode;

int findtoken(const struct tnode * tk, const char * str, void** result);
void addtoken(struct tnode * root, const char* str, void * id);

#ifdef __cplusplus
}
#endif
#endif
