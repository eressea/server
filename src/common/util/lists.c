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


#include <stdlib.h>
#include <assert.h>

#include <config.h>
#include "lists.h"

void
addlist(void *l1, void *p1)
{

	/* add entry p to the end of list l */

	void_list **l;
	void_list *p, *q;

	l = l1;
	p = p1;
	assert(p->next == 0);

	if (*l) {
		for (q = *l; q->next; q = q->next)
			assert(q);
		q->next = p;
	} else
		*l = p;
}

void
choplist(void * a, void * b)
{
	void_list **l = (void_list**)a, *p = (void_list*)b;
	/* remove entry p from list l - when called, a pointer to p must be
	 * kept in order to use (and free) p; if omitted, this will be a
	 * memory leak */

	void_list **q;

	for (q = l; *q; q = &((*q)->next)) {
		if (*q == p) {
			*q = p->next;
			p->next = 0;
			break;
		}
	}
}

void
translist(void *l1, void *l2, void *p)
{

	/* remove entry p from list l1 and add it at the end of list l2 */

	choplist(l1, p);
	addlist(l2, p);
}

void
insertlist(void_list ** l, void_list * p)
{

	/* insert entry p at the beginning of list l */

	p->next = *l;
	*l = p;

}

void
promotelist(void *l, void *p)
{

	/* remove entry p from list l; insert p again at the beginning of l */

	choplist(l, p);
	insertlist(l, p);
}

#ifndef MALLOCDBG
void
removelist(void *l, void *p)
{

	/* remove entry p from list l; free p */

	choplist(l, p);
	free(p);
}

void
freelist(void *p1)
{

	/* remove all entries following and including entry p from a listlist */

	void_list *p, *p2;

	p = p1;

	while (p) {
		p2 = p->next;
		free(p);
		p = p2;
	}
}
#endif

void
invert_list(void * heap)
{
	void_list * x = NULL;
	void_list * m = *(void_list**)heap;
	while (m)
	{
		void_list * d = m;
		m = m->next;
		d->next = x;
		x = d;
	}
	*(void **)heap = x;
}

size_t
listlen(void *l)
{

	/* count entries p in list l */

	size_t i;
	void_list *p;

	for (p = l, i = 0; p; p = p->next, i++);
	return i;
}

/* Hilfsfunktion, um das Debugging zu erleichtern.  Statt print
 * (cast)foo->next->next->next->next nur noch
 * print (cast)listelem(foo, 3) */

void *
listelem(void *l, int n)
{
	int i=0;

	while(i < n && l != NULL) {
			l = ((void_list *)l)->next;
			i++;
	}

	return l;
}


