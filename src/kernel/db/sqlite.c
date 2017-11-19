#include <platform.h>

#include <kernel/config.h>
#include <kernel/database.h>
#include <kernel/orderdb.h>

#include <util/log.h>

#include "driver.h"

#include <sqlite3.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *g_db;
static sqlite3_stmt * g_stmt_insert;
static sqlite3_stmt * g_stmt_select;

static int g_order_batchsize;
static int g_order_tx_size;

order_data *db_driver_order_load(int id)
{
    order_data * od = NULL;
    int err;
    
    if (g_order_tx_size > 0) {
        g_order_tx_size = 0;
        err = sqlite3_exec(g_db, "COMMIT", NULL, NULL, NULL);
        assert(err == SQLITE_OK);
    }
    err = sqlite3_reset(g_stmt_select);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_int(g_stmt_select, 1, id);
    assert(err == SQLITE_OK);
    do {
        err = sqlite3_step(g_stmt_select);
        if (err == SQLITE_ROW) {
            const unsigned char *text;
            int bytes;
            bytes = sqlite3_column_bytes(g_stmt_select, 0);
            assert(bytes > 0);
            text = sqlite3_column_text(g_stmt_select, 0);
            odata_create(&od, 1+(size_t)bytes, (const char *)text);
            return od;
        }
    } while (err == SQLITE_ROW);
    assert(err == SQLITE_DONE);
    return NULL;
}

int db_driver_order_save(order_data *od)
{
    int err;
    sqlite3_int64 id;
    
    assert(od && od->_str);
    
    if (g_order_batchsize > 0) {
        if (g_order_tx_size == 0) {
            err = sqlite3_exec(g_db, "BEGIN TRANSACTION", NULL, NULL, NULL);
            assert(err == SQLITE_OK);
        }
    }
    
    err = sqlite3_reset(g_stmt_insert);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(g_stmt_insert, 1, od->_str, -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_step(g_stmt_insert);
    assert(err == SQLITE_DONE);
    id = sqlite3_last_insert_rowid(g_db);
    assert(id <= INT_MAX);
    
    if (g_order_batchsize > 0) {
        if (++g_order_tx_size >= g_order_batchsize) {
            err = sqlite3_exec(g_db, "COMMIT", NULL, NULL, NULL);
            assert(err == SQLITE_OK);
            g_order_tx_size = 0;
        }
    }
    
    return (int)id;
}

void db_driver_open(void)
{
    int err;
    const char *dbname;

    g_order_batchsize = config_get_int("game.dbbatch", 100);
    dbname = config_get("game.dbname");
    if (!dbname) {
        dbname = "";
    }
    err = sqlite3_open(dbname, &g_db);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_db, "PRAGMA journal_mode=OFF", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_db, "PRAGMA synchronous=OFF", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_db, "CREATE TABLE IF NOT EXISTS orders (id INTEGER PRIMARY KEY, data TEXT NOT NULL)", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_db, "INSERT INTO orders (data) VALUES (?)", -1, &g_stmt_insert, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_db, "SELECT data FROM orders WHERE id = ?", -1, &g_stmt_select, NULL);
    assert(err == SQLITE_OK);
}

void db_driver_close(void)
{
    int err;

    err = sqlite3_finalize(g_stmt_select);
    assert(err == SQLITE_OK);
    err = sqlite3_finalize(g_stmt_insert);
    assert(err == SQLITE_OK);
    err = sqlite3_close(g_db);
    assert(err == SQLITE_OK);
}
