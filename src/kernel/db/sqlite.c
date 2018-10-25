#include <platform.h>

#include <kernel/config.h>

#include <util/log.h>

#include "driver.h"

#include <sqlite3.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static sqlite3 *g_game_db;
static sqlite3 *g_swap_db;
static sqlite3_stmt * g_stmt_insert_string;
static sqlite3_stmt * g_stmt_select_string;
static sqlite3_stmt * g_stmt_insert_order;
static sqlite3_stmt * g_stmt_select_order;
static sqlite3_stmt * g_stmt_update_faction;
static sqlite3_stmt * g_stmt_insert_faction;

static int g_insert_batchsize;
static int g_insert_tx_size;

static void end_transaction(void) {
    if (g_insert_tx_size > 0) {
        int err;
        g_insert_tx_size = 0;
        err = sqlite3_exec(g_swap_db, "COMMIT", NULL, NULL, NULL);
        assert(err == SQLITE_OK);
    }
}

struct order_data *db_driver_order_load(dbrow_id id)
{
    struct order_data * od = NULL;
    int err;

    ERRNO_CHECK();
    end_transaction();
    err = sqlite3_reset(g_stmt_select_order);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_int(g_stmt_select_order, 1, id);
    assert(err == SQLITE_OK);
    err = sqlite3_step(g_stmt_select_order);
    if (err == SQLITE_ROW) {
        const unsigned char *text;
        int bytes;
        text = sqlite3_column_text(g_stmt_select_order, 0);
        bytes = sqlite3_column_bytes(g_stmt_select_order, 0);
        assert(bytes > 0);
        odata_create(&od, 1+(size_t)bytes, (const char *)text);
        ERRNO_CHECK();
        return od;
    }
    ERRNO_CHECK();
    return NULL;
}

dbrow_id db_driver_order_save(const char *str) {
    int err;
    sqlite3_int64 id;
    
    assert(str);
   
    ERRNO_CHECK();

    if (g_insert_batchsize > 0) {
        if (g_insert_tx_size == 0) {
            err = sqlite3_exec(g_swap_db, "BEGIN TRANSACTION", NULL, NULL, NULL);
            assert(err == SQLITE_OK);
        }
    }
    
    err = sqlite3_reset(g_stmt_insert_order);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(g_stmt_insert_order, 1, str, -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_step(g_stmt_insert_order);
    assert(err == SQLITE_DONE);
    id = sqlite3_last_insert_rowid(g_swap_db);
    assert(id > 0 && id <= UINT_MAX);
    
    if (g_insert_batchsize > 0) {
        if (++g_insert_tx_size >= g_insert_batchsize) {
            end_transaction();
        }
    }
    ERRNO_CHECK(); 
    return (dbrow_id)id;
}


dbrow_id db_driver_faction_save(dbrow_id id, int no, int turn, const char *email, const char *password)
{
    sqlite3_int64 row_id;
    int err;

    assert(g_game_db);
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
    assert(row_id>0 && row_id <= UINT_MAX);
    return (dbrow_id)row_id;
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

    g_insert_batchsize = config_get_int("game.dbbatch", 100);

    err = sqlite3_open(dbname, &g_swap_db);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_swap_db, "PRAGMA journal_mode=OFF", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_swap_db, "PRAGMA synchronous=OFF", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_swap_db, "CREATE TABLE IF NOT EXISTS orders (id INTEGER PRIMARY KEY, data TEXT NOT NULL)", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_swap_db, "CREATE TABLE IF NOT EXISTS strings (id INTEGER PRIMARY KEY, data TEXT NOT NULL)", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_swap_db, "INSERT INTO strings (data) VALUES (?)", -1, &g_stmt_insert_string, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_swap_db, "SELECT data FROM strings WHERE id=?", -1, &g_stmt_select_string, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_swap_db, "INSERT INTO orders (data) VALUES (?)", -1, &g_stmt_insert_order, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_swap_db, "SELECT data FROM orders WHERE id=?", -1, &g_stmt_select_order, NULL);
    assert(err == SQLITE_OK);
    ERRNO_CHECK();
    return 0;
}

static const char *g_swapname;

int db_driver_open(database_t db, const char *dbname)
{
    ERRNO_CHECK();
    
    if (!dbname) {
        /* by default, use an in-memory database */
        dbname = ":memory:";
    }
    if (db == DB_SWAP) {
        g_swapname = dbname;
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
        assert(g_swap_db);
        err = sqlite3_finalize(g_stmt_select_string);
        assert(err == SQLITE_OK);
        err = sqlite3_finalize(g_stmt_insert_string);
        assert(err == SQLITE_OK);
        err = sqlite3_finalize(g_stmt_select_order);
        assert(err == SQLITE_OK);
        err = sqlite3_finalize(g_stmt_insert_order);
        assert(err == SQLITE_OK);
        err = sqlite3_close(g_swap_db);
        assert(err == SQLITE_OK);
        if (g_swapname) {
            FILE * F = fopen(g_swapname, "r");
            if (F) {
                fclose(F);
                remove(g_swapname);
            }
        }
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

dbrow_id db_driver_string_save(const char *str) {
    int err;
    sqlite3_int64 id;

    assert(str);

    ERRNO_CHECK();

    if (g_insert_batchsize > 0) {
        if (g_insert_tx_size == 0) {
            err = sqlite3_exec(g_swap_db, "BEGIN TRANSACTION", NULL, NULL, NULL);
            assert(err == SQLITE_OK);
        }
    }

    err = sqlite3_reset(g_stmt_insert_string);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(g_stmt_insert_string, 1, str, -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_step(g_stmt_insert_string);
    assert(err == SQLITE_DONE);
    id = sqlite3_last_insert_rowid(g_swap_db);
    assert(id > 0 && id <= UINT_MAX);

    if (g_insert_batchsize > 0) {
        if (++g_insert_tx_size >= g_insert_batchsize) {
            end_transaction();
        }
    }
    ERRNO_CHECK();
    return (dbrow_id)id;
}

const char *db_driver_string_load(dbrow_id id, size_t *size) {
    int err;

    end_transaction();
    err = sqlite3_reset(g_stmt_select_string);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_int(g_stmt_select_string, 1, id);
    assert(err == SQLITE_OK);
    err = sqlite3_step(g_stmt_select_string);
    if (err == SQLITE_ROW) {
        const unsigned char *text;
        text = sqlite3_column_text(g_stmt_select_string, 0);
        if (size) {
            int bytes = sqlite3_column_bytes(g_stmt_select_string, 0);
            assert(bytes > 0);
            *size = (size_t)bytes;
        }
        ERRNO_CHECK();
        return (const char *)text;
    }
    ERRNO_CHECK();
    return NULL;
}
