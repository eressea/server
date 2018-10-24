#include "config.h"
#include "database.h"
#include "db/driver.h"

void swapdb_open(void)
{
    const char *dbname;
    dbname = config_get("game.dbswap");
    db_driver_open(DB_SWAP, dbname);
}

void swapdb_close(void)
{
    db_driver_close(DB_SWAP);
}

dbstring_id db_string_save(const char *s) {
    (void)s;
    return 0;
}

dbstring_id dbstring_save(const char *s) {
    return db_driver_string_save(s);
}

const char *dbstring_load(dbstring_id id, size_t *size) {
    return db_driver_string_load(id, size);
}
