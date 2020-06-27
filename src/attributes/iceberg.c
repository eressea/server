#include <platform.h>
#include <kernel/config.h>
#include "iceberg.h"

#include <kernel/attrib.h>

attrib_type at_iceberg = {
    "iceberg_drift",
    NULL,
    NULL,
    NULL,
    a_writeint,
    a_readint,
    NULL,
    ATF_UNIQUE
};

attrib *make_iceberg(direction_t dir)
{
    attrib *a = a_new(&at_iceberg);
    a->data.i = (int)dir;
    return a;
}
