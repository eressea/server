#include <platform.h>

#include "kernel/config.h"
#include "kernel/db/driver.h"

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
