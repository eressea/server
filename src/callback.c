#include <platform.h>
#include "callback.h"
#include <stdlib.h>
#include <string.h>

static struct reg {
    struct reg * next;
    HCALLBACK cb;
    char *name;
} *registry;

HCALLBACK create_callback(void(*cbv)(va_list va)) {
    HCALLBACK cb;
    cb.cbv = cbv;
    return cb;
}

void reset_callbacks(void) {
    while (registry) {
        struct reg *r = registry;
        registry = r->next;
        free(r->name);
        free(r);
    }
    registry = 0;
}

HCALLBACK register_callback(const char *name, void(*cbv)(va_list va))
{
    struct reg * r = (struct reg *)malloc(sizeof(struct reg));
    r->next = registry;
    r->name = _strdup(name);
    r->cb.cbv = cbv;
    registry = r;
    return r->cb;
}

int find_callback(const char *name, HCALLBACK *result) {
    if (result && name) {
        struct reg *r;
        for (r = registry; r; r = r->next) {
            if (strcmp(r->name, name) == 0) {
                *result = r->cb;
                return 0;
            }
        }
    }
    return -1;
}

int call_callback(HCALLBACK cb, const char *name, ...) {
    va_list ap;
    if (name) {
        int err = find_callback(name, &cb);
        if (err) return err;
    }
    va_start(ap, name);
    cb.cbv(ap);
    va_end(ap);
    return 0;
}
