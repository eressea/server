#ifndef H_ORDERDB
#define H_ORDERDB

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        DB_NONE,
        DB_MEMORY,
        DB_MMAP,
        DB_BDB,
        DB_SQLITE
    } db_backend;

    typedef struct order_data {
        const char *_str;
        int _refcount;
    } order_data;

    db_backend orderdb_open(db_backend choices[]);
    void orderdb_close(void);

    void odata_create(order_data **pdata, size_t len, const char *str);
    void odata_release(order_data * od);

    order_data *odata_load(int id);
    int odata_save(order_data *od);

#ifdef __cplusplus
}
#endif
#endif
