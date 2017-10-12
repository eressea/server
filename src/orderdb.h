#ifndef H_ORDERDB
#define H_ORDERDB

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct order_data {
        const char *_str;
        int _refcount;
    } order_data;

    void orderdb_open(void);
    void orderdb_close(void);

    order_data *odata_load(int id);
    int odata_save(order_data *od);
    void odata_release(order_data * od);

#ifdef __cplusplus
}
#endif
#endif
