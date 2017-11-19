#include <platform.h>
#include "driver.h"

#include <kernel/orderdb.h>
#include <util/log.h>

#include <critbit.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static critbit_tree cb_orders = { 0 };
static int auto_id = -1;

struct cb_entry {
    int id;
    order_data *data;
};

order_data *db_driver_order_load(int id)
{
    void * match;

    assert(id>0);
    if (cb_find_prefix(&cb_orders, &id, sizeof(id), &match, 1, 0) > 0) {
        struct cb_entry *ent = (struct cb_entry *)match;
        order_data * od = ent->data;
        ++od->_refcount;
        return od;
    }
    return NULL;
}

int db_driver_order_save(order_data *od)
{
    struct cb_entry ent;

    assert(od && od->_str);
    ++od->_refcount;
    ent.id = ++auto_id;
    ent.data = od;
    cb_insert(&cb_orders, &ent, sizeof(ent));
    return ent.id;
}

static int free_data_cb(const void *match, const void *key, size_t keylen,
    void *udata)
{
    struct cb_entry * ent = (struct cb_entry *)match;
    order_data *od = ent->data;
    odata_release(od);
    return 0;
}

void db_driver_open(void)
{
    assert(auto_id == -1);
    auto_id = 0;
}

void db_driver_close(void)
{
    cb_foreach(&cb_orders, NULL, 0, free_data_cb, NULL);
    cb_clear(&cb_orders);
    auto_id = -1;
}
