#ifndef H_ATTRIBUTE_ICEBERG
#define H_ATTRIBUTE_ICEBERG

#include "direction.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_iceberg;

    struct attrib *make_iceberg(direction_t dir);

#ifdef __cplusplus
}
#endif
#endif
