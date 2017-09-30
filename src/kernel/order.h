/*
 +-------------------+
 |                   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Enno Rehling <enno@eressea.de>
 | (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
 |                   |
 +-------------------+

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef KRNL_ORDER_H
#define KRNL_ORDER_H

#include "keyword.h"
#include <stddef.h>
#include <stdbool.h>

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

    struct order_data;

#define CMD_QUIET   0x010000
#define CMD_PERSIST 0x020000
#define CMD_DEFAULT 0x040000

    typedef struct order {
        struct order *next;
        /* do not access this data: */
        struct order_data *data;
        int command;
    } order;

    /* constructor */
    order *create_order(keyword_t kwd, const struct locale *lang,
        const char *params, ...);
    order *parse_order(const char *s, const struct locale *lang);
    void replace_order(order ** dst, order * orig, const order * src);

    /* reference counted copies of orders: */
    order *copy_order(const order * ord);
    void free_order(order * ord);
    void free_orders(order ** olist);

    void push_order(struct order **olist, struct order *ord);

    /* access functions for orders */
    keyword_t getkeyword(const order * ord);
    void set_order(order ** destp, order * src);
    char* get_command(const order *ord, char *buffer, size_t size);
    bool is_persistent(const order * ord);
    bool is_silent(const order * ord);
    bool is_exclusive(const order * ord);
    bool is_repeated(keyword_t kwd);
    bool is_long(keyword_t kwd);

    char *write_order(const order * ord, char *buffer, size_t size);
    keyword_t init_order(const struct order *ord);

    void close_orders(void);

#ifdef __cplusplus
}
#endif
#endif
