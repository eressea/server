#include <platform.h>
#include "db.h"
#include "orderdb.h"

#include <util/log.h>

#include <critbit.h>

#include <stdlib.h>
#include <string.h>

static critbit_tree cb_orders = { 0 };
static int auto_id = 0;

struct cb_entry {
    int id;
    order_data *data;
};

order_data *db_load_order(int id)
{
    void * match;

    if (id > 0) {
        if (cb_find_prefix(&cb_orders, &id, sizeof(id), &match, 1, 0) > 0) {
            struct cb_entry *ent = (struct cb_entry *)match;
            order_data * od = ent->data;
            ++od->_refcount;
            return od;
        }
    }
    return NULL;
}

int db_save_order(order_data *od)
{
    if (od->_str) {
        struct cb_entry ent;
        ++od->_refcount;
        ent.id = ++auto_id;
        ent.data = od;
        cb_insert(&cb_orders, &ent, sizeof(ent));
        return ent.id;
    }
    return 0;
}

static int free_data_cb(const void *match, const void *key, size_t keylen,
    void *udata)
{
    struct cb_entry * ent = (struct cb_entry *)match;
    order_data *od = ent->data;
    if (od->_refcount > 1) {
        log_error("refcount=%d for order %d, %s", od->_refcount, ent->id, od->_str);
    }
    odata_release(od);
    return 0;
}

void db_open(void)
{
    auto_id = 0;
}

void db_close(void)
{
    cb_foreach(&cb_orders, NULL, 0, free_data_cb, NULL);
    cb_clear(&cb_orders);
}
