/* vi: set ts=2:
 *
 *	$Id: lists.h,v 1.2 2001/01/26 16:19:41 enno Exp $
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

#ifndef LISTS_H
#define LISTS_H

#include <stddef.h>

typedef struct list list;
struct list {
	list * next;
	void * data;
};

#define list_foreach(type, list, item) item=list; while (item!=NULL) { type* __next__=item->next;
#define list_continue(item) { item=__next__; continue; }
#define list_next(item) item=__next__; }

void addlist(void *l1, void *p1);
void choplist(void * l, void * p);
void translist(void *l1, void *l2, void *p);
void promotelist(void *l, void *p);
#ifndef MALLOCDBG
void freelist(void *p1);
void removelist(void *l, void *p);
#else
#define freelist(p) { while (p) { void * p2 = p->next; free(p); p = p2; } }
#define removelist(l,p) { choplist(l, p); free(p); }
#endif

size_t listlen(void *l);
void invert_list(void * heap);
#define addlist2(l, p)       (*l = p, l = &p->next)

void *listelem(void *l, int n);

#endif
