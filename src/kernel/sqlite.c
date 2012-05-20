#include <platform.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <util/unicode.h>
#include <util/base36.h>
#include <util/log.h>
#include <sqlite3.h>
#include <md5.h>
#include <assert.h>

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

#define SQL_EXPECT(res, val) if (res!=val) { return val; }
#define MAX_EMAIL_LENGTH 64
#define MD5_LENGTH 32
#define MD5_LENGTH_0 (MD5_LENGTH+1)     /* MD5 + zero-terminator */
#define MAX_FACTION_NAME 64
#define MAX_FACTION_NAME_0 (MAX_FACTION_NAME+1)
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

  for (i = 0; i != MAX_STMT_CACHE && cache[i].db; ++i) {
    if (cache[i].sql == sql && cache[i].db == db) {
      cache[i].inuse = 1;
      sqlite3_reset(cache[i].stmt);
      return cache[i].stmt;
    }
  }
  if (i == MAX_STMT_CACHE) {
    while (cache[cache_insert].inuse) {
      cache[cache_insert].inuse = 0;
      cache_insert = (cache_insert + 1) & (MAX_STMT_CACHE - 1);
    }
    i = cache_insert;
    sqlite3_finalize(cache[i].stmt);
  }
  cache[i].inuse = 1;
  cache[i].db = db;
  cache[i].sql = sql;
  sqlite3_prepare_v2(db, sql, -1, &cache[i].stmt, NULL);
  return cache[i].stmt;
}

typedef struct db_faction {
  sqlite3_uint64 id_faction;
  sqlite3_uint64 id_email;
  int no;
  const char *email;
  const char *passwd_md5;
  const char *name;
} db_faction;

static int
db_update_email(sqlite3 * db, const faction * f, const db_faction * dbstate,
  boolean force, /* [OUT] */ sqlite3_uint64 * id_email)
{
  boolean update = force;
  int res = SQLITE_OK;
  char email_lc[MAX_EMAIL_LENGTH];

  if (update) {
    unicode_utf8_tolower(email_lc, sizeof(email_lc), f->email);
  } else {
    if (strcmp(dbstate->email, f->email) != 0) {
      unicode_utf8_tolower(email_lc, sizeof(email_lc), f->email);
      if (strcmp(dbstate->email, email_lc) != 0) {
        update = true;
      }
    }
  }

  if (update) {
    char email_md5[MD5_LENGTH_0];
    int i;
    md5_state_t ms;
    md5_byte_t digest[16];
    const char sql_insert_email[] =
      "INSERT OR FAIL INTO email (email,md5) VALUES (?,?)";
    const char sql_select_email[] = "SELECT id FROM email WHERE md5=?";
    sqlite3_stmt *stmt_insert_email = stmt_cache_get(db, sql_insert_email);
    sqlite3_stmt *stmt_select_email = stmt_cache_get(db, sql_select_email);

    md5_init(&ms);
    md5_append(&ms, (md5_byte_t *) email_lc, (int)strlen(email_lc));
    md5_finish(&ms, digest);
    for (i = 0; i != 16; ++i)
      sprintf(email_md5 + 2 * i, "%.02x", digest[i]);

    res =
      sqlite3_bind_text(stmt_insert_email, 1, email_lc, -1, SQLITE_TRANSIENT);
    res =
      sqlite3_bind_text(stmt_insert_email, 2, email_md5, MD5_LENGTH,
      SQLITE_TRANSIENT);
    res = sqlite3_step(stmt_insert_email);

    if (res == SQLITE_DONE) {
      *id_email = sqlite3_last_insert_rowid(db);
    } else {
      res =
        sqlite3_bind_text(stmt_select_email, 1, email_md5, MD5_LENGTH,
        SQLITE_TRANSIENT);
      res = sqlite3_step(stmt_select_email);
      SQL_EXPECT(res, SQLITE_ROW);
      *id_email = sqlite3_column_int64(stmt_select_email, 0);
    }
  }

  return SQLITE_OK;
}

int db_update_factions(sqlite3 * db, boolean force)
{
  int game_id = 6;
  const char sql_select[] =
    "SELECT faction.id, faction.email_id, faction.code, email.email, faction.password_md5, faction.name, faction.lastturn FROM email, faction"
    " WHERE email.id=faction.email_id AND faction.game_id=? AND (lastturn IS NULL OR lastturn>?)";
  sqlite3_stmt *stmt_select = stmt_cache_get(db, sql_select);
  faction *f;
  int res;

  res = sqlite3_bind_int(stmt_select, 1, game_id);
  SQL_EXPECT(res, SQLITE_OK);
  res = sqlite3_bind_int(stmt_select, 2, turn - 2);
  SQL_EXPECT(res, SQLITE_OK);
  for (;;) {
    sqlite3_uint64 id_faction;
    int lastturn;

    res = sqlite3_step(stmt_select);
    if (res != SQLITE_ROW)
      break;

    id_faction = sqlite3_column_int64(stmt_select, 0);
    lastturn = sqlite3_column_int(stmt_select, 6);
    f = get_faction_by_id((int)id_faction);

    if (f == NULL || !f->alive) {
      if (lastturn == 0) {
        const char sql_update[] = "UPDATE faction SET lastturn=? WHERE id=?";
        sqlite3_stmt *stmt = stmt_cache_get(db, sql_update);

        lastturn = f ? f->lastorders : turn - 1;
        sqlite3_bind_int(stmt, 1, lastturn);
        sqlite3_bind_int64(stmt, 2, id_faction);
        res = sqlite3_step(stmt);
        SQL_EXPECT(res, SQLITE_DONE);
      }
    } else {
      md5_state_t ms;
      md5_byte_t digest[16];
      int i;
      char passwd_md5[MD5_LENGTH_0];
      sqlite3_uint64 id_email;
      boolean update = force;
      db_faction dbstate;
      const char *no_b36;

      fset(f, FFL_MARK);
      dbstate.id_faction = id_faction;
      dbstate.id_email = sqlite3_column_int64(stmt_select, 1);
      no_b36 = (const char *)sqlite3_column_text(stmt_select, 2);
      dbstate.no = no_b36 ? atoi36(no_b36) : -1;
      dbstate.email = (const char *)sqlite3_column_text(stmt_select, 3);
      dbstate.passwd_md5 = (const char *)sqlite3_column_text(stmt_select, 4);
      dbstate.name = (const char *)sqlite3_column_text(stmt_select, 5);

      id_email = dbstate.id_email;
      res = db_update_email(db, f, &dbstate, force, &id_email);
      SQL_EXPECT(res, SQLITE_OK);

      md5_init(&ms);
      md5_append(&ms, (md5_byte_t *) f->passw, (int)strlen(f->passw));
      md5_finish(&ms, digest);
      for (i = 0; i != 16; ++i)
        sprintf(passwd_md5 + 2 * i, "%.02x", digest[i]);

      if (!update) {
        update = ((id_email != 0 && dbstate.id_email != id_email)
          || dbstate.no != f->no || dbstate.passwd_md5 == NULL
          || strcmp(passwd_md5, dbstate.passwd_md5) != 0 || dbstate.name == NULL
          || strncmp(f->name, dbstate.name, MAX_FACTION_NAME) != 0);
      }
      if (update) {
        const char sql_update_faction[] =
          "UPDATE faction SET email_id=?, password_md5=?, code=?, name=?, firstturn=? WHERE id=?";
        sqlite3_stmt *stmt_update_faction =
          stmt_cache_get(db, sql_update_faction);

        res = sqlite3_bind_int64(stmt_update_faction, 1, id_email);
        SQL_EXPECT(res, SQLITE_OK);
        res =
          sqlite3_bind_text(stmt_update_faction, 2, passwd_md5, MD5_LENGTH,
          SQLITE_TRANSIENT);
        SQL_EXPECT(res, SQLITE_OK);
        res =
          sqlite3_bind_text(stmt_update_faction, 3, no_b36, -1,
          SQLITE_TRANSIENT);
        SQL_EXPECT(res, SQLITE_OK);
        res =
          sqlite3_bind_text(stmt_update_faction, 4, f->name, -1,
          SQLITE_TRANSIENT);
        SQL_EXPECT(res, SQLITE_OK);
        res = sqlite3_bind_int(stmt_update_faction, 5, turn - f->age);
        SQL_EXPECT(res, SQLITE_OK);
        res = sqlite3_bind_int64(stmt_update_faction, 6, f->subscription);
        SQL_EXPECT(res, SQLITE_OK);
        res = sqlite3_step(stmt_update_faction);
        SQL_EXPECT(res, SQLITE_DONE);
      }
    }
  }

  for (f = factions; f; f = f->next) {
    if (!fval(f, FFL_MARK)) {
      log_error("%s (sub=%d, email=%s) has no entry in the database\n", factionname(f), f->subscription, f->email);
    } else {
      freset(f, FFL_MARK);
    }
  }
  return SQLITE_OK;
}

int db_update_scores(sqlite3 * db, boolean force)
{
  const char *sql_ins =
    "INSERT OR FAIL INTO score (value,faction_id,turn) VALUES (?,?,?)";
  sqlite3_stmt *stmt_ins = stmt_cache_get(db, sql_ins);
  const char *sql_upd =
    "UPDATE score set value=? WHERE faction_id=? AND turn=?";
  sqlite3_stmt *stmt_upd = stmt_cache_get(db, sql_upd);
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
  return SQLITE_OK;
}
