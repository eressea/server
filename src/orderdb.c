#include <platform.h>
#include "orderdb.h"

#include <util/log.h>

#include <critbit.h>

#include <stdlib.h>

static critbit_tree cb_orders = { 0 };
static int auto_id = 0;

struct cb_entry {
    int id;
    order_data *data;
};

static db_backend odata_backend = DB_NONE;

order_data *odata_load(int id)
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

int odata_save(order_data *od)
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

void odata_release(order_data * od)
{
    if (od) {
        if (--od->_refcount == 0) {
            free(od);
        }
    }
}

int free_data_cb(const void *match, const void *key, size_t keylen, void *udata) {
    struct cb_entry * ent = (struct cb_entry *)match;
    order_data *od = ent->data;
    if (od->_refcount > 1) {
        log_error("refcount=%d for order %d, %s", od->_refcount, ent->id, od->_str);
    }
    odata_release(od);
    return 0;
}

void free_data(void) {
    cb_foreach(&cb_orders, NULL, 0, free_data_cb, NULL);
    cb_clear(&cb_orders);
}

db_backend orderdb_open(db_backend choices[])
{
    int i;
    for (i = 0; choices[i] != DB_NONE; ++i) {
        db_backend choice = choices[i];
        if (choice == DB_MEMORY) {
            auto_id = 0;
            return odata_backend = choice;
        }
    }
    return DB_NONE;
}

void orderdb_close(void)
{
    free_data();
}
