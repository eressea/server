#include "log.h"

#include "path.h"
#include "strings.h"
#include "unicode.h"

#include <critbit.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static critbit_tree stats = CRITBIT_TREE();

int stats_count(const char* stat, int delta) {
    void* match;
    if (cb_find_prefix_str(&stats, stat, &match, 1, 0) == 0) {
        size_t len;
        char data[128];
        len = cb_new_kv(stat, strlen(stat), &delta, sizeof(delta), data);
        cb_insert(&stats, data, len);
        return delta;
    }
    else {
        int* num;
        cb_get_kv_ex(match, (void**)&num);
        return *num += delta;
    }
}

struct walk_data {
    int (*callback)(const char*, int, void*);
    void* udata;
};

static int walk_cb(void* match, const void* key, size_t keylen, void* udata) {
    struct walk_data* data = (struct walk_data*)udata;
    int* num;

    (void)key;
    (void)keylen;
    cb_get_kv_ex(match, (void**)&num);
    return data->callback((const char*)match, *num, data->udata);
}

int stats_walk(const char* prefix, int (*callback)(const char*, int, void*), void* udata) {
    struct walk_data data;
    data.callback = callback;
    data.udata = udata;
    return cb_foreach(&stats, prefix, strlen(prefix), walk_cb, &data);
}

static int write_cb(const char* key, int val, void* udata) {
    FILE* F = (FILE*)udata;
    fprintf(F, "%s: %d\n", (const char*)key, val);
    return 0;
}

void stats_write(FILE* F, const char* prefix) {
    stats_walk(prefix, write_cb, F);
}

void stats_close(void) {
    cb_clear(&stats);
}
