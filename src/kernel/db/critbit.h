#pragma once

struct order_data;

void db_critbit_open(void);
void db_critbit_close(void);
int db_critbit_order_save(struct order_data *od);
struct order_data *db_critbit_order_load(int id);
