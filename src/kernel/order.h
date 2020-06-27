#ifndef KRNL_ORDER_H
#define KRNL_ORDER_H

#include <util/keyword.h>

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct stream;

    /* Encapsulation of an order
     *
     * This structure contains one order given by a unit. These used to be
     * stored in string lists, but by storing them in order-structures,
     * it is possible to use reference-counting on them, reduce string copies,
     * and reduce overall memory usage by sharing strings between orders (not
     * implemented yet) saving approx. 50% of all string-related memory.
     */

#define CMD_QUIET   0x010000
#define CMD_PERSIST 0x020000
#define CMD_DEFAULT 0x040000

    typedef struct order_data {
        const char *_str;
        int _refcount;
    } order_data;

    extern order_data *odata_load(int id);
    extern int odata_save(order_data *od);

    void odata_create(order_data **pdata, size_t len, const char *str);
    void odata_release(order_data * od);
    void odata_addref(order_data *od);

    typedef struct order {
        struct order *next;
        /* do not access this data: */
        int id;
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
    char* get_command(const order *ord, const struct locale *lang,
        char *buffer, size_t size);
    bool is_persistent(const order * ord);
    bool is_silent(const order * ord);
    bool is_exclusive(const order * ord);
    bool is_repeated(keyword_t kwd);
    bool is_long(keyword_t kwd);
    const char *crescape(const char *str, char *buffer, size_t size);

    char *write_order(const order * ord, const struct locale *lang,
        char *buffer, size_t size);
    int stream_order(struct stream *out, const struct order *ord, const struct locale *lang, bool escape);
    keyword_t init_order_depr(const struct order *ord);
    keyword_t init_order(const struct order *ord, const struct locale *lang);

    void close_orders(void);

#ifdef __cplusplus
}
#endif
#endif
