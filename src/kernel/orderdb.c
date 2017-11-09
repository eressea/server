#include <platform.h>
#include "db.h"
#include "orderdb.h"

#include <util/log.h>

#include <critbit.h>

#include <stdlib.h>
#include <string.h>

void odata_create(order_data **pdata, size_t len, const char *str)
{
    order_data *data;
    char *result;

    data = malloc(sizeof(order_data) + len + 1);
    data->_refcount = 1;
    result = (char *)(data + 1);
    data->_str = (len > 0) ? result : NULL;
    if (str) strcpy(result, str);
    if (pdata) *pdata = data;
}

void odata_release(order_data * od)
{
    if (od) {
        if (--od->_refcount == 0) {
            free(od);
        }
    }
}

order_data *odata_load(int id)
{
    return db_load_order(id);
}

int odata_save(order_data *od)
{
    return db_save_order(od);
}
