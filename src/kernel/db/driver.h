#pragma once

#include <stddef.h>

struct order_data;

typedef unsigned int dbrow_id;

extern void odata_create(struct order_data **pdata, size_t len, const char *str);

typedef enum database_t {
    DB_SWAP,
    DB_GAME,
} database_t;

int db_driver_open(database_t db, const char *dbname);
void db_driver_close(database_t db);
dbrow_id db_driver_order_save(const char *str);
struct order_data *db_driver_order_load(dbrow_id id);
dbrow_id db_driver_faction_save(dbrow_id id, int no, int turn, const char *email, const char *password);
dbrow_id db_driver_string_save(const char *s);
const char *db_driver_string_load(dbrow_id id, size_t *size);
