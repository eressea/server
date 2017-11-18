#pragma once

struct order_data;

void db_sqlite_open(void);
void db_sqlite_close(void);
int db_sqlite_order_save(struct order_data *od);
struct order_data *db_sqlite_order_load(int id);
