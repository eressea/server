/* vi: set ts=2:
 *
 *	$Id: umlaut.h,v 1.1 2001/01/25 09:37:58 enno Exp $
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

#ifndef _UMLAUT_H
#define _UMLAUT_H
typedef struct tnode tnode;
struct tnode {
	struct tnode * nexthash;
	struct tnode * next[32];
	void * id;
	char c;
	unsigned char leaf;
};

#define E_TOK_NOMATCH NULL

void * findtoken(struct tnode * tk, const char * str);
void addtoken(struct tnode * root, const char* str, void * id);

#endif
