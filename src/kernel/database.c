#include <platform.h>
#include "database.h"

#include <platform.h>

#include <kernel/config.h>
#include <kernel/faction.h>
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

int dblib_save_faction(const faction *f, int turn) {
    return db_driver_faction_save(f->uid, f->no, turn, f->email, f->_password);
}

void dblib_open(void)
{
    db_driver_open();
}

void dblib_close(void)
{
    db_driver_close();
}
