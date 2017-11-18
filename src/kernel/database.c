#include <platform.h>
#include "database.h"

#include <platform.h>

#include <kernel/config.h>
#include <kernel/database.h>
#include <kernel/orderdb.h>

#include <util/log.h>

#ifdef USE_SQLITE
#include "db/sqlite.h"
#else
#include "db/critbit.h"
#endif

order_data *db_load_order(int id)
{
    if (id > 0) {
#ifdef USE_SQLITE
        return db_sqlite_order_load(id);
#else
        return db_critbit_order_load(id);
#endif
    }
    return NULL;
}

int db_save_order(order_data *od)
{
    if (od->_str) {
#ifdef USE_SQLITE
        return db_sqlite_order_save(od);
#else
        return db_critbit_order_save(od);
#endif
    }
    return 0;
}

void db_open(void)
{
#ifdef USE_SQLITE
    db_sqlite_open();
#else
    db_critbit_open();
#endif
}

void db_close(void)
{
#ifdef USE_SQLITE
    db_sqlite_close();
#else
    db_critbit_close();
#endif
}
