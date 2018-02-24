#include <sys/types.h>
#include <db.h>

#include <platform.h>
#include "driver.h"

#include <kernel/config.h>
#include <kernel/orderdb.h>

#include <assert.h>
#include <string.h>

static DB *g_dbp;

void db_driver_open(void)
{
    int ret;
    u_int32_t flags = DB_CREATE;
    const char * dbname;

    dbname = config_get("game.dbname");
    ret = db_create(&g_dbp, NULL, 0);
    assert(ret==0);

    ret = g_dbp->open(g_dbp, NULL, dbname, NULL, DB_RECNO, flags, 0);
    assert(ret==0);
}

void db_driver_close(void)
{
    int ret;
    ret = g_dbp->close(g_dbp, 0);
    assert(ret==0);
}

int db_driver_order_save(struct order_data *od)
{
    int ret;
    DBT key, data;
    db_recno_t recno;

    assert(od && od->_str);
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    key.data = &recno;
    key.size = key.ulen = sizeof(recno);
    key.flags = DB_DBT_USERMEM;
    data.data = (void *)od->_str;
    data.size = data.ulen = strlen(od->_str) + 1;
    data.flags = DB_DBT_USERMEM;
    ret = g_dbp->put(g_dbp, NULL, &key, &data, DB_APPEND);
    assert(ret == 0);
    return (int)recno;
}

struct order_data *db_driver_order_load(int id)
{
    int ret;
    order_data *od = NULL;
    DBT key, data;
    db_recno_t recno;

    assert(id>0);
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    recno = (db_recno_t)id;
    key.data = &recno;
    key.size = key.ulen = sizeof(recno);
    key.flags = DB_DBT_USERMEM;
    ret = g_dbp->get(g_dbp, NULL, &key, &data, 0);
    if (ret == 0) {
        odata_create(&od, data.size, data.data);
    }
    return od;
}

