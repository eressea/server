#include <platform.h>
#include "db.h"
#include "orderdb.h"

#include <util/log.h>

#include <sqlite3.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>


#define DBNAME ":memory:"
static sqlite3 *g_db;
static sqlite3_stmt * g_stmt_insert;
static sqlite3_stmt * g_stmt_select;

order_data *db_load_order(int id)
{
    if (id > 0) {
        order_data * od = NULL;
        odata_create(&od, 0, NULL);
        return od;
    }
    return NULL;
}

int db_save_order(order_data *od)
{
    if (od->_str) {
        int err;
        err = sqlite3_reset(g_stmt_insert);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_text(g_stmt_insert, 1, od->_str, -1, SQLITE_STATIC);
        assert(err == SQLITE_OK);
        err = sqlite3_step(g_stmt_insert);
        assert(err == SQLITE_DONE);
    }
    return 0;
}

void db_open(void)
{
    int err;

    err = sqlite3_open(DBNAME, &g_db);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_db, "CREATE TABLE IF NOT EXISTS orders (id INTEGER PRIMARY KEY, data TEXT NOT NULL)", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_db, "INSERT INTO orders (data) VALUES (?)", -1, &g_stmt_insert, NULL);
    assert(err == SQLITE_OK);
}

void db_close(void)
{
    int err;

    err = sqlite3_finalize(g_stmt_insert);
    assert(err == SQLITE_OK);
    err = sqlite3_close(g_db);
    assert(err == SQLITE_OK);
}
