#include <platform.h>
#include "database.h"

#include <platform.h>

#include <kernel/config.h>
#include <kernel/database.h>
#include <kernel/orderdb.h>

#include <util/log.h>

#include "db/driver.h"

order_data *dblib_load_order(int id)
{
    if (id > 0) {
        return db_driver_order_load(id);
    }
    return NULL;
}

int dblib_save_order(order_data *od)
{
    if (od->_str) {
        return db_driver_order_save(od);
    }
    return 0;
}

void dblib_open(void)
{
    db_driver_open();
}

void dblib_close(void)
{
    db_driver_close();
}
