#ifndef H_DATABASE
#define H_DATABASE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct order_data;

    void dblib_open(void);
    void dblib_close(void);

    struct order_data *dblib_load_order(int id);
    int dblib_save_order(struct order_data *od);

#ifdef __cplusplus
}
#endif
#endif
