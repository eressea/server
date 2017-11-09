#include <platform.h>
#include "db.h"
#include "orderdb.h"

#include <util/log.h>

#include <sqlite3.h>

#include <assert.h>
#include <limits.h>
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
        int err;

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
                odata_create(&od, (size_t)bytes, (const char *)text);
                return od;
            }
        } while (err == SQLITE_ROW);
        assert(err == SQLITE_DONE);
    }
    return NULL;
}

int db_save_order(order_data *od)
{
    if (od->_str) {
        int err;
        sqlite3_int64 id;
        err = sqlite3_reset(g_stmt_insert);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_text(g_stmt_insert, 1, od->_str, -1, SQLITE_STATIC);
        assert(err == SQLITE_OK);
        err = sqlite3_step(g_stmt_insert);
        assert(err == SQLITE_DONE);
        id = sqlite3_last_insert_rowid(g_db);
        assert(id <= INT_MAX);
        return (int)id;
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
    err = sqlite3_prepare_v2(g_db, "SELECT data FROM orders WHERE id = ?", -1, &g_stmt_select, NULL);
    assert(err == SQLITE_OK);
}

void db_close(void)
{
    int err;

    err = sqlite3_finalize(g_stmt_select);
    assert(err == SQLITE_OK);
    err = sqlite3_finalize(g_stmt_insert);
    assert(err == SQLITE_OK);
    err = sqlite3_close(g_db);
    assert(err == SQLITE_OK);
}
