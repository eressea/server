#pragma once

#include <stddef.h>
#include <stdbool.h>

struct order_data;

typedef unsigned int dbrow_id;

void odata_create(struct order_data **pdata, size_t len, const char *str);

typedef enum database_t {
    DB_SWAP,
    DB_GAME,
} database_t;

int db_driver_open(database_t db, const char *dbname);
void db_driver_close(database_t db);
dbrow_id db_driver_order_save(const char *str);
struct order_data *db_driver_order_load(dbrow_id id);
dbrow_id db_driver_string_save(const char *s);
const char *db_driver_string_load(dbrow_id id, size_t *size);
void db_driver_compact(int turn);

typedef struct db_faction {
    int *p_uid;
    int no;
    const char *email;
    char pwhash[128];
} db_faction;

typedef int (*db_faction_generator)(void *, db_faction *, int);
int db_driver_update_factions(db_faction_generator gen, void *data);
int db_driver_faction_save(dbrow_id * p_id, int no, const char *email, const char *password);
