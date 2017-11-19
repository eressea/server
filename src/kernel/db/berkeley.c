#include <platform.h>
#include "driver.h"

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#endif
#include <db.h>

#include <assert.h>

static DB *g_dbp;

void db_driver_open(void)
{
    int ret;
    u_int32_t flags = DB_CREATE|DB_TRUNCATE;
    const char * dbname;

    ret = db_create(&g_dbp, NULL, 0);
    assert(ret==0);

    ret = g_dbp->open(g_dbp, NULL, dbname, NULL, DB_BTREE, flags, 0);
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
    return 0;
}

struct order_data *db_driver_order_load(int id)
{
    return NULL;
}

