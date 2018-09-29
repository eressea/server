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

static sqlite3 *g_game_db;
static sqlite3 *g_temp_db;
static sqlite3_stmt * g_stmt_insert_order;
static sqlite3_stmt * g_stmt_select_order;
static sqlite3_stmt * g_stmt_update_faction;
static sqlite3_stmt * g_stmt_insert_faction;

static int g_order_batchsize;
static int g_order_tx_size;

order_data *db_driver_order_load(int id)
{
    order_data * od = NULL;
    int err;

    ERRNO_CHECK();
    if (g_order_tx_size > 0) {
        g_order_tx_size = 0;
        err = sqlite3_exec(g_temp_db, "COMMIT", NULL, NULL, NULL);
        assert(err == SQLITE_OK);
    }
    err = sqlite3_reset(g_stmt_select_order);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_int(g_stmt_select_order, 1, id);
    assert(err == SQLITE_OK);
    do {
        err = sqlite3_step(g_stmt_select_order);
        if (err == SQLITE_ROW) {
            const unsigned char *text;
            int bytes;
            bytes = sqlite3_column_bytes(g_stmt_select_order, 0);
            assert(bytes > 0);
            text = sqlite3_column_text(g_stmt_select_order, 0);
            odata_create(&od, 1+(size_t)bytes, (const char *)text);
            ERRNO_CHECK();
            return od;
        }
    } while (err == SQLITE_ROW);
    assert(err == SQLITE_DONE);
    ERRNO_CHECK();
    return NULL;
}

int db_driver_order_save(order_data *od)
{
    int err;
    sqlite3_int64 id;
    
    assert(od && od->_str);
   
    ERRNO_CHECK();

    if (g_order_batchsize > 0) {
        if (g_order_tx_size == 0) {
            err = sqlite3_exec(g_temp_db, "BEGIN TRANSACTION", NULL, NULL, NULL);
            assert(err == SQLITE_OK);
        }
    }
    
    err = sqlite3_reset(g_stmt_insert_order);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(g_stmt_insert_order, 1, od->_str, -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_step(g_stmt_insert_order);
    assert(err == SQLITE_DONE);
    id = sqlite3_last_insert_rowid(g_temp_db);
    assert(id <= INT_MAX);
    
    if (g_order_batchsize > 0) {
        if (++g_order_tx_size >= g_order_batchsize) {
            err = sqlite3_exec(g_temp_db, "COMMIT", NULL, NULL, NULL);
            assert(err == SQLITE_OK);
            g_order_tx_size = 0;
        }
    }
    ERRNO_CHECK(); 
    return (int)id;
}


int db_driver_faction_save(int id, int no, int turn, const char *email, const char *password)
{
    sqlite3_int64 row_id;
    int err;

    if (!g_game_db) {
        return -1;
    }
    if (id != 0) {
        int rows;

        err = sqlite3_reset(g_stmt_update_faction);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_int(g_stmt_update_faction, 1, no);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_int(g_stmt_update_faction, 2, turn);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_text(g_stmt_update_faction, 3, email, -1, SQLITE_STATIC);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_text(g_stmt_update_faction, 4, password, -1, SQLITE_STATIC);
        assert(err == SQLITE_OK);
        err = sqlite3_bind_int(g_stmt_update_faction, 5, id);
        assert(err == SQLITE_OK);
        err = sqlite3_step(g_stmt_update_faction);
        assert(err == SQLITE_DONE);
        rows = sqlite3_changes(g_game_db);
        if (rows != 0) {
            return id;
        }
    }
    err = sqlite3_reset(g_stmt_insert_faction);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_int(g_stmt_insert_faction, 1, no);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_int(g_stmt_insert_faction, 2, turn);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(g_stmt_insert_faction, 3, email, -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(g_stmt_insert_faction, 4, password, -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_step(g_stmt_insert_faction);
    assert(err == SQLITE_DONE);
    ERRNO_CHECK();

    row_id = sqlite3_last_insert_rowid(g_game_db);
    assert(row_id <= INT_MAX);
    return (int)row_id;
}

static int db_open_game(const char *dbname) {
    int err;

    err = sqlite3_open(dbname, &g_game_db);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_game_db, "CREATE TABLE IF NOT EXISTS factions (id INTEGER PRIMARY KEY, no INTEGER NOT NULL, email VARCHAR(128), password VARCHAR(128), turn INTEGER NOT NULL)", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_game_db, "UPDATE factions SET no=?, turn=?, email=?, password=? WHERE id=?", -1, &g_stmt_update_faction, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_game_db, "INSERT INTO factions (no, turn, email, password) VALUES (?,?,?,?)", -1, &g_stmt_insert_faction, NULL);
    assert(err == SQLITE_OK);

    ERRNO_CHECK();
    return 0;
}

static int db_open_swap(const char *dbname) {
    int err;

    g_order_batchsize = config_get_int("game.dbbatch", 100);

    err = sqlite3_open(dbname, &g_temp_db);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_temp_db, "PRAGMA journal_mode=OFF", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_temp_db, "PRAGMA synchronous=OFF", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_temp_db, "CREATE TABLE IF NOT EXISTS orders (id INTEGER PRIMARY KEY, data TEXT NOT NULL)", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_temp_db, "INSERT INTO orders (data) VALUES (?)", -1, &g_stmt_insert_order, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_temp_db, "SELECT data FROM orders WHERE id=?", -1, &g_stmt_select_order, NULL);
    assert(err == SQLITE_OK);
    ERRNO_CHECK();
    return 0;
}

int db_driver_open(database_t db, const char *dbname)
{
    ERRNO_CHECK();

    if (db == DB_SWAP) {
        return db_open_swap(dbname);
    }
    else if (db == DB_GAME) {
        return db_open_game(dbname);
    }
    return -1;
}

void db_driver_close(database_t db)
{
    int err;

    ERRNO_CHECK();
    if (db == DB_SWAP) {
        assert(g_temp_db);
        err = sqlite3_finalize(g_stmt_select_order);
        assert(err == SQLITE_OK);
        err = sqlite3_finalize(g_stmt_insert_order);
        assert(err == SQLITE_OK);
        err = sqlite3_close(g_temp_db);
        assert(err == SQLITE_OK);
    }
    else if (db == DB_GAME) {
        assert(g_game_db);
        err = sqlite3_finalize(g_stmt_update_faction);
        assert(err == SQLITE_OK);
        err = sqlite3_finalize(g_stmt_insert_faction);
        assert(err == SQLITE_OK);
        err = sqlite3_close(g_game_db);
        assert(err == SQLITE_OK);
    }
    ERRNO_CHECK();
}

