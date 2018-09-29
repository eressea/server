#ifdef _MSC_VER
#include <platform.h>
#endif
#include "driver.h"

#include <util/log.h>
#include <util/strings.h>

#include <selist.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static selist * g_orders;
static int auto_id = -1;

struct order_data *db_driver_order_load(int id)
{
    void * match;

    assert(id>0);
    match = selist_get(g_orders, id - 1);
    if (match) {
        char * str = (char *)match;
        struct order_data * od = NULL;
        odata_create(&od, strlen(str), str);
        return od;
    }
    return NULL;
}

int db_driver_order_save(const char * str)
{
    assert(str);
    selist_push(&g_orders, str_strdup(str));
    return ++auto_id;
}

int db_driver_faction_save(int id, int no, int turn, const char *email, const char *password)
{
    return -1;
}

static void free_data_cb(void *match)
{
    char *str = (char *)match;
    free(str);
}

int db_driver_open(database_t db, const char *dbname)
{
    (void)dbname;
    if (db == DB_SWAP) {
        assert(auto_id == -1);
        auto_id = 0;
        return 0;
    }
    return -1;
}

void db_driver_close(database_t db)
{
    if (db == DB_SWAP) {
        selist_foreach(g_orders, free_data_cb);
        selist_free(g_orders);
        g_orders = NULL;
        auto_id = -1;
    }
}
