#ifndef H_DB
#define H_DB

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct order_data;

    void db_open(void);
    void db_close(void);

    struct order_data *db_load_order(int id);
    int db_save_order(struct order_data *od);

#ifdef __cplusplus
}
#endif
#endif
