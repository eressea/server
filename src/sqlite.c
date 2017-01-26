#include <platform.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <util/language.h>
#include <util/log.h>
#include <util/base36.h>
#include <util/log.h>
#include <selist.h>
#include <sqlite3.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

faction *get_faction_by_id(int uid)
{
    faction *f;
    for (f = factions; f; f = f->next) {
        if (f->subscription == uid) {
            return f;
        }
    }
    return NULL;
}

/*
typedef struct stmt_cache {
sqlite3 *db;
sqlite3_stmt *stmt;
const char *sql;
int inuse;
} stmt_cache;

#define MAX_STMT_CACHE 64
static stmt_cache cache[MAX_STMT_CACHE];
static int cache_insert;

static sqlite3_stmt *stmt_cache_get(sqlite3 * db, const char *sql)
{
int i;
sqlite3_stmt *stmt;

for (i = 0; i != MAX_STMT_CACHE && cache[i].db; ++i) {
if (cache[i].sql == sql && cache[i].db == db) {
cache[i].inuse = 1;
stmt = cache[i].stmt;
sqlite3_reset(stmt);
sqlite3_clear_bindings(stmt);
return stmt;
}
}
if (i == MAX_STMT_CACHE) {
while (cache[cache_insert].inuse) {
cache[cache_insert].inuse = 0;
cache_insert = (cache_insert + 1) & (MAX_STMT_CACHE - 1);
}
i = cache_insert;
stmt = cache[i].stmt;
sqlite3_finalize(stmt);
}
cache[i].inuse = 1;
cache[i].db = db;
cache[i].sql = sql;
sqlite3_prepare_v2(db, sql, -1, &cache[i].stmt, NULL);
return cache[i].stmt;
}
*/
typedef struct db_faction {
    int uid;
    int no;
    char *email;
    char *name;
} db_faction;

static struct selist *
read_factions(sqlite3 * db, int game_id) {
    int res;
    selist *result = 0;
    const char * sql =
        "SELECT f.id, fd.code, fd.name, fd.email FROM faction f"
        " LEFT OUTER JOIN faction_data fd"
        " WHERE f.id=fd.faction_id AND f.game_id=? AND"
        " fd.turn=(SELECT MAX(turn) FROM faction_data fx WHERE fx.faction_id=f.id)"
        " ORDER BY f.id";
    sqlite3_stmt *stmt = 0;
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, game_id);

    res = sqlite3_step(stmt);
    while (res == SQLITE_ROW) {
        const char * text;
        db_faction * dbf = (db_faction*)calloc(1, sizeof(db_faction));
        dbf->uid = (int)sqlite3_column_int64(stmt, 0);
        text = (const char *)sqlite3_column_text(stmt, 1);
        if (text) dbf->no = atoi36(text);
        text = (const char *)sqlite3_column_text(stmt, 2);
        if (text) dbf->name = strdup(text);
        text = (const char *)sqlite3_column_text(stmt, 3);
        if (text) dbf->email = strdup(text);
        selist_push(&result, dbf);
        res = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

static int insert_faction(sqlite3 *db, int game_id, faction *f) {
    const char *sql = "INSERT INTO faction (game_id, race) VALUES (?, ?)";
    sqlite3_stmt *stmt = 0;
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, game_id);
    sqlite3_bind_text(stmt, 2, f->race->_name, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (int)sqlite3_last_insert_rowid(db);
}

static void update_faction(sqlite3 *db, const faction *f) {
    char code[5];
    const char *sql =
        "INSERT INTO faction_data (faction_id, code, name, email, lang, turn)"
        " VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt = 0;
    strcpy(code, itoa36(f->no));
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, f->subscription);
    sqlite3_bind_text(stmt, 2, code, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, f->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, f->email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, locale_name(f->locale), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, turn);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int db_update_factions(sqlite3 * db, bool force, int game_id) {
    selist *ql = read_factions(db, game_id);
    faction *f;
    sqlite3_exec(db, "BEGIN", 0, 0, 0);
    for (f = factions; f; f = f->next) {
        bool update = force;
        db_faction *dbf = 0;
<<<<<<< HEAD
        ql_iter it = qli_init(&ql);
=======
#ifdef SELIST_TODO
        selist_iter it = qli_init(&ql);
>>>>>>> remove quicklist shim, use selist everywhere
        while (qli_more(it)) {
            db_faction *df = (db_faction*)qli_next(&it);
            if (f->no == df->no || strcmp(f->email, df->email) == 0 || strcmp(f->name, df->name) == 0) {
                dbf = df;
            }
            if (f->subscription == df->uid) {
                dbf = df;
                break;
            }
        }
        if (dbf) {
            if (dbf->uid != f->subscription) {
                log_warning("faction %s(%d) not found in database, but matches %d\n", itoa36(f->no), f->subscription, dbf->uid);
                f->subscription = dbf->uid;
            }
            update = (dbf->no != f->no) || (strcmp(f->email, dbf->email) != 0) || (strcmp(f->name, dbf->name) != 0);
        }
        else {
            f->subscription = insert_faction(db, game_id, f);
            log_warning("faction %s not found in database, created as %d\n", itoa36(f->no), f->subscription);
            update = true;
        }
        if (update) {
            update_faction(db, f);
            log_debug("faction %s updated\n", itoa36(f->no));
        }
    }
    sqlite3_exec(db, "COMMIT", 0, 0, 0);
    return SQLITE_OK;
}

int db_update_scores(sqlite3 * db, bool force)
{
    /*
    const char *sselist_ins =
    "INSERT OR FAIL INTO score (value,faction_id,turn) VALUES (?,?,?)";
    sqlite3_stmt *stmt_ins = stmt_cache_get(db, sselist_ins);
    const char *sselist_upd =
    "UPDATE score set value=? WHERE faction_id=? AND turn=?";
    sqlite3_stmt *stmt_upd = stmt_cache_get(db, sselist_upd);
    faction *f;
    sqlite3_exec(db, "BEGIN", 0, 0, 0);
    for (f = factions; f; f = f->next) {
    int res;
    sqlite3_bind_int(stmt_ins, 1, f->score);
    sqlite3_bind_int64(stmt_ins, 2, f->subscription);
    sqlite3_bind_int(stmt_ins, 3, turn);
    res = sqlite3_step(stmt_ins);
    if (res == SQLITE_CONSTRAINT) {
    sqlite3_bind_int(stmt_upd, 1, f->score);
    sqlite3_bind_int64(stmt_upd, 2, f->subscription);
    sqlite3_bind_int(stmt_upd, 3, turn);
    res = sqlite3_step(stmt_upd);
    sqlite3_reset(stmt_upd);
    }
    sqlite3_reset(stmt_ins);
    }
    sqlite3_exec(db, "COMMIT", 0, 0, 0);
    */
    return SQLITE_OK;
}
