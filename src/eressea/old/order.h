/* vi: set ts=2:
 *
 *	$Id: order.h,v 1.1 2001/01/27 18:15:32 enno Exp $
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
#ifndef ORDER_H
#define ORDER_H

#define order_t int

typedef union {
	void * v;
	int i;
	char c;
	short s;
	short sa[2];
	char ca[4];
} variant;

typedef struct order {
	struct order * next;
	order_t type;
	variant data;
} order;

typedef struct orders {
	order * list;
	order ** end;
} orders;

extern void append_order(orders * os, order * o);
extern order * new_order(const char * cmd);
extern void free_order(order * o);
extern const char * order_string(const order * o, char * buffer, size_t buffersize);

#endif
