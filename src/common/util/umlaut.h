/* vi: set ts=2:
 *
 *	$Id: umlaut.h,v 1.3 2001/02/15 02:41:47 enno Exp $
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

#define E_TOK_NOMATCH (-1)
#define E_TOK_SUCCESS 0

struct tref;

typedef struct tnode {
	struct tref * next[32];
	unsigned char flags;
	void * id;
} tnode;

int findtoken(struct tnode * tk, const char * str, void** result);
void addtoken(struct tnode * root, const char* str, void * id);

#endif
