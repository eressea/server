/* vi: set ts=2:
 *
 *	$Id: umlaut.c,v 1.5 2001/02/15 02:41:47 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "umlaut.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct tref {
	struct tref * nexthash;
	char c;
	struct tnode * node;
} tref;

#define LEAF 1 /* leaf node for a word. always matches */
#define SHARED 2 /* at least two words share the node */

void
addtoken(tnode * root, const char* str, void * id)
{
	static char zText[1024];
	static struct replace {
		char c;
		const char * str;
	} replace[] = {
		{'ä', "ae"},
		{'Ä', "ae"},
		{'ö', "oe"},
		{'Ö', "oe"},
		{'ü', "ue"},
		{'Ü', "ue"},
		{'ß', "ss"},
		{ 0, 0 }
	};
	if (!*str) {
		root->id = id;
		root->flags |= LEAF;
	} else {
		tref * next;
		int index, i = 0;
		char c = *str;
		if (c<'a' || c>'z') c = (char)tolower((unsigned char)c);
		index = ((unsigned char)c) % 32;
		next = root->next[index];
		if (!root->flags) root->id = id;
		while (next && next->c != c) next = next->nexthash;
		if (!next) {
			tref * ref;
			char u = (char)toupper((unsigned char)c);
			tnode * node = calloc(1, sizeof(tnode));

			ref = malloc(sizeof(tref));
			ref->c = c;
			ref->node = node;
			ref->nexthash=root->next[index];
			root->next[index] = ref;
			
			if (u!=c) {
				index = ((unsigned char)u) % 32;
				ref = malloc(sizeof(tref));
				ref->c = u;
				ref->node = node;
				ref->nexthash = root->next[index];
				root->next[index] = ref;
			}
			next=ref;
		} else {
			next->node->flags |= SHARED;
			if (next->node->flags & LEAF == 0) next->node->id = NULL;
		}
		addtoken(next->node, str+1, id);
		while (replace[i].str) {
			if (*str==replace[i].c) {
				strcat(strcpy(zText, replace[i].str), str+1);
				addtoken(root, zText, id);
				break;
			}
			++i;
		}
	}
}

int
findtoken(tnode * tk, const char * str, void **result)
{
	if (*str == 0) return E_TOK_NOMATCH;

	while (*str) {
		int index;
		tref * ref;
		char c = *str;

/*		if (c<'a' || c>'z') c = (char)tolower((unsigned char)c); */

		index = ((unsigned char)c) % 32;
		ref = tk->next[index];
		while (ref && ref->c!=c) ref = ref->nexthash;
		++str;
		if (!ref) return E_TOK_NOMATCH;
		tk = ref->node;
		if (tk && !(tk->flags & SHARED)) {
			*result = tk->id;
			return E_TOK_SUCCESS;
		}
	}
	if (tk && (tk->flags & LEAF)) {
		*result = tk->id;
		return E_TOK_SUCCESS;
	}
	return E_TOK_NOMATCH;
}
