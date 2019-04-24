#include <platform.h>

#include <kernel/config.h>

#include <util/log.h>
#include <util/base36.h>

#include "driver.h"

#include <sqlite3.h>

#include <assert.h>
#include <errno.h>
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

int db_driver_faction_save(dbrow_id * p_id, int no, const char *email, const char *password)
{
    dbrow_id id = *p_id;
    int err;
    sqlite3_stmt *stmt = (id > 0) ? g_stmt_update_faction : g_stmt_insert_faction;

    assert(g_game_db);

    err = sqlite3_reset(stmt);
    if (err != SQLITE_OK) return err;
    err = sqlite3_bind_text(stmt, 1, itoa36(no), -1, SQLITE_STATIC);
    if (err != SQLITE_OK) return err;
    err = sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
    if (err != SQLITE_OK) return err;
    err = sqlite3_bind_text(stmt, 3, password, -1, SQLITE_STATIC);
    if (err != SQLITE_OK) return err;

    if (id > 0) {
        err = sqlite3_bind_int(stmt, 4, id);
        if (err != SQLITE_OK) return err;
    }
    err = sqlite3_step(stmt);
    if (err != SQLITE_DONE) return err;
    ERRNO_CHECK();

    if (id <= 0) {
        sqlite3_int64 row_id;
        row_id = sqlite3_last_insert_rowid(g_game_db);
        assert(row_id > 0 && row_id <= UINT_MAX);
        *p_id = (dbrow_id)row_id;
    }
    return SQLITE_OK;
}

int db_driver_update_factions(db_faction_generator gen, void *data)
{
    db_faction results[32];
    int num, err;

    err = sqlite3_exec(g_game_db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_exec(g_game_db, "DELETE FROM `faction`", NULL, NULL, NULL);
    if (err != SQLITE_OK) {
        sqlite3_exec(g_game_db, "ROLLBACK", NULL, NULL, NULL);
        return err;
    }
    num = gen(data, results, 32);
    while (num > 0) {
        int i;
        for (i = 0; i != num; ++i) {
            db_faction *dbf = results + i;
            dbrow_id id = (dbrow_id)*dbf->p_uid;
            err = db_driver_faction_save(&id, dbf->no, dbf->email, dbf->pwhash);
            if (err != SQLITE_OK) {
                sqlite3_exec(g_game_db, "ROLLBACK", NULL, NULL, NULL);
                return err;
            }
            assert(id > 0 && id <= INT_MAX);
            *dbf->p_uid = (int)id;
        }
        num = gen(data, results, 32);
    }
    err = sqlite3_exec(g_game_db, "COMMIT", NULL, NULL, NULL);
    return err;
}

static int cb_int_col(void *data, int ncols, char **text, char **name) {
    int *p_int = (int *)data;
    *p_int = atoi(text[0]);
    return SQLITE_OK;
}

static int db_open_game(const char *dbname) {
    int err, version = 0;

    err = sqlite3_open(dbname, &g_game_db);
    assert(err == SQLITE_OK);

    err = sqlite3_exec(g_game_db, "PRAGMA user_version", cb_int_col, &version, NULL);
    assert(err == SQLITE_OK);
    if (version < 1) {
        /* drop deprecated table */
        err = sqlite3_exec(g_game_db, "DROP TABLE IF EXISTS `factions`", NULL, NULL, NULL);
        assert(err == SQLITE_OK);
    }
    if (version < 2) {
        /* install schema version 2: */
        err = sqlite3_exec(g_game_db, "CREATE TABLE IF NOT EXISTS `faction` (`id` INTEGER PRIMARY KEY, `no` CHAR(4) NOT NULL UNIQUE, `email` VARCHAR(128), `password` VARCHAR(128))", NULL, NULL, NULL);
        assert(err == SQLITE_OK);
        err = sqlite3_exec(g_game_db, "PRAGMA user_version = 2", NULL, NULL, NULL);
        assert(err == SQLITE_OK);
    }
    /* create prepared statments: */
    err = sqlite3_prepare_v2(g_game_db, "INSERT INTO `faction` (`no`, `email`, `password`, `id`) VALUES (?,?,?,?)", -1, &g_stmt_update_faction, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_prepare_v2(g_game_db, "INSERT INTO `faction` (`no`, `email`, `password`) VALUES (?,?,?)", -1, &g_stmt_insert_faction, NULL);
    assert(err == SQLITE_OK);

    ERRNO_CHECK();
    return err;
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
                if (0 != remove(g_swapname)) {
                    log_error("could not remove %s: %s", g_swapname,
                            strerror(errno));
                    errno = 0;
                }
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

void db_driver_compact(int turn)
{
    int err;

    /* repack database: */
    err = sqlite3_exec(g_game_db, "VACUUM", 0, 0, 0);
    assert(err == SQLITE_OK);
}

