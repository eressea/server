#ifndef H_GC_MORALE
#define H_GC_MORALE

#include <kernel/types.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct region;
    void morale_update(struct region *r);
    void morale_change(struct region *r, int value);

#ifdef __cplusplus
}
#endif
#endif
