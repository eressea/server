#include <platform.h>
#include "driver.h"

#include <kernel/config.h>
#include <kernel/orderdb.h>

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#endif
#include <db.h>

#include <assert.h>
#include <string.h>

static DB *g_dbp;
static int order_id = -1;

void db_driver_open(void)
{
    int ret;
    u_int32_t flags = DB_CREATE|DB_TRUNCATE;
    const char * dbname;

    dbname = config_get("game.dbname");
    if (!dbname) {
        dbname = "temporary.db";
    }
    ret = db_create(&g_dbp, NULL, 0);
    assert(ret==0);

    ret = g_dbp->open(g_dbp, NULL, dbname, NULL, DB_BTREE, flags, 0);
    assert(ret==0);

    assert(order_id == -1);
    order_id = 0;
}

void db_driver_close(void)
{
    int ret;
    ret = g_dbp->close(g_dbp, 0);
    assert(ret==0);
    order_id = -1;
}

int db_driver_order_save(struct order_data *od)
{
    int ret;
    DBT key, data;

    assert(od && od->_str);
    ++order_id;
    key.data = &order_id;
    key.size = sizeof(int);
    data.data = (void *)od->_str;
    data.size = strlen(od->_str) + 1;
    data.flags = DB_DBT_USERMEM;
    ret = g_dbp->put(g_dbp, NULL, &key, &data, DB_NOOVERWRITE);
    assert(ret == 0);
    return order_id;
}

struct order_data *db_driver_order_load(int id)
{
    int ret;
    order_data *od = NULL;
    DBT key, data;

    assert(id>0 && id <= order_id);
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    key.data = &id;
    key.size = sizeof(int);
    ret = g_dbp->get(g_dbp, NULL, &key, &data, 0);
    if (ret == 0) {
        odata_create(&od, data.ulen, data.data);
    }
    return od;
}

