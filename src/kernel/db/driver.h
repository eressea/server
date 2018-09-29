#pragma once

struct order_data;

typedef enum database_t {
    DB_SWAP,
    DB_GAME,
} database_t;

int db_driver_open(database_t db, const char *dbname);
void db_driver_close(database_t db);
int db_driver_order_save(struct order_data *od);
struct order_data *db_driver_order_load(int id);
int db_driver_faction_save(int id, int no, int turn, const char *email, const char *password);
