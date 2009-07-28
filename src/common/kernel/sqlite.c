#include <config.h>
#include <kernel/eressea.h>
#include <kernel/faction.h>
#include <sqlite3.h>
#include <md5.h>

int
db_update_factions(sqlite3 * db)
{
  const char * sql = "INSERT OR REPLACE INTO email (md5, email) VALUES(?, ?)";
  faction * f;
  sqlite3_stmt *stmt;
  int res = sqlite3_prepare_v2(db, sql, strlen(sql)+1, &stmt, NULL);
  if (res!=SQLITE_OK) {
    return res;
  }
  for (f=factions;f;f=f->next) {
    char hash[33];
    md5_state_t ms;
    md5_byte_t digest[16];
    int i;
    sqlite3_int64 rowid;
    
    md5_init(&ms);
    md5_append(&ms, (md5_byte_t*)f->email, strlen(f->email));
    md5_finish(&ms, digest);
    for (i=0;i!=16;++i) sprintf(hash+2*i, "%.02x", digest[i]);
    
    res = sqlite3_bind_text(stmt, 1, hash, -1, SQLITE_TRANSIENT);
    res = sqlite3_bind_text(stmt, 2, f->email, -1, SQLITE_TRANSIENT);
    do {
      res = sqlite3_step(stmt);
    } while (res!=SQLITE_DONE);
    rowid = sqlite3_last_insert_rowid(db);
    // TODO: use rowid to update faction    
    sqlite3_reset(stmt);
  }
  sqlite3_finalize(stmt);
  return SQLITE_OK;
}
