/* vi: set ts=2:
 *
 *	$Id: umlaut.c,v 1.2 2001/01/26 16:19:41 enno Exp $
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

void
addtoken(tnode * root, const char* str, void * id)
{
	static char buf[1024];
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
	assert(id!=E_TOK_NOMATCH);
	if (root->id!=E_TOK_NOMATCH && root->id!=id && !root->leaf) root->id=E_TOK_NOMATCH;
	if (!*str) {
		root->id = id;
		root->leaf=1;
	} else {
		char c = (char)tolower(*(unsigned char*)str);
		int index = ((unsigned char)c) % 32;
		int i=0;
		tnode * tk = root->next[index];
		if (root->id==E_TOK_NOMATCH) root->id = id;
		while (tk && tk->c != c) tk = tk->nexthash;
		if (!tk) {
			tk = calloc(1, sizeof(tnode));
			tk->id = E_TOK_NOMATCH;
			tk->c = c;
			tk->nexthash=root->next[index];
			root->next[index] = tk;
		}
		addtoken(tk, str+1, id);
		while (replace[i].str) {
			if (*str==replace[i].c) {
				strcat(strcpy(buf, replace[i].str), str+1);
				addtoken(root, buf, id);
				break;
			}
			++i;
		}
	}
}

void *
findtoken(tnode * tk, const char * str)
{
	if(*str == 0) return E_TOK_NOMATCH;

	while (*str) {
		char c = (char)tolower(*str);
		int index = ((unsigned char)c) % 32;
		tk = tk->next[index];
		while (tk && tk->c!=c) tk = tk->nexthash;
		++str;
		if (!tk) return E_TOK_NOMATCH;
	}
	return tk->id;
}

