#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

    int stats_count(const char *stat, int delta);
    void stats_write(FILE *F, const char *prefix);
    int stats_walk(const char *prefix, int (*callback)(const char *key, int val, void * udata), void *udata);
    void stats_close(void);

#ifdef __cplusplus
}
#endif
