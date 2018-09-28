#include <platform.h>

#include "config.h"
#include "db/driver.h"

#include "orderdb.h"

#include <util/log.h>

#include <critbit.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void orderdb_open(void)
{
    const char *dbname;

    dbname = config_get("game.dbswap");
    db_driver_open(DB_SWAP, dbname);
}

void orderdb_close(void)
{
    db_driver_close(DB_SWAP);
}

void odata_create(order_data **pdata, size_t len, const char *str)
{
    order_data *data;
    char *result;

    assert(pdata);
    data = malloc(sizeof(order_data) + len + 1);
    data->_refcount = 1;
    result = (char *)(data + 1);
    data->_str = (len > 0) ? result : NULL;
    if (str) {
        strcpy(result, str);
    }
    *pdata = data;
}

void odata_release(order_data * od)
{
    if (od) {
        if (--od->_refcount == 0) {
            free(od);
        }
    }
}

void odata_addref(order_data *od)
{
    ++od->_refcount;
}

order_data *odata_load(int id)
{
    if (id > 0) {
        return db_driver_order_load(id);
    }
    return NULL;
}

int odata_save(order_data *od)
{
    if (od->_str) {
        return db_driver_order_save(od->_str);
    }
    return 0;
}
