/* vi: set ts=2:
 * Eressea PB(E)M host   Christian Schlittchen (corwin@amber.kn-bremen.de)
 * (C) 1998-2003         Katja Zedel (katze@felidae.kn-bremen.de)
 *                       Henning Peters (faroul@beyond.kn-bremen.de)
 *                       Enno Rehling (enno@eressea-pbem.de)
 *                       Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 **/

#ifndef KRNL_ORDER_H
#define KRNL_ORDER_H
#ifdef __cplusplus
extern "C" {
#endif

/* Encapsulation of an order
 *
 * This structure contains one order given by a unit. These used to be
 * stored in string lists, but by storing them in order-structures,
 * it is possible to use reference-counting on them, reduce string copies,
 * and reduce overall memory usage by sharing strings between orders (not
 * implemented yet) saving approx. 50% of all string-related memory.
 */

typedef struct order {
	struct order * next;
	char * str;
	keyword_t keyword;
	int refcount;
} order;

/* constructor */
extern struct order * parse_order(const char * s, const struct locale * lang);

/* reference counted copies of orders: */
extern struct order * copy_order(struct order * ord);
extern void free_order(struct order * ord);
extern void free_orders(struct order ** olist);

/* access functions for orders */
extern keyword_t getkeyword(const struct order * ord);
extern void set_order(struct order ** destp, struct order * src);
extern const char * getcommand(const struct order * ord);
extern boolean is_persistent(const struct order *ord);
extern char * write_order(const struct order * ord, const struct locale * lang, char * buffer, size_t size);
#ifdef __cplusplus
}
#endif
#endif
