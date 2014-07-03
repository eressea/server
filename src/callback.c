#include "callback.h"
#include <stdlib.h>
#include <string.h>

static struct reg {
    struct reg * next;
    CALLBACK cb;
    const char *name;
} *registry;

CALLBACK create_callback(void (*cbv)(va_list va)) {
    CALLBACK cb;
    cb.cbv = cbv;
    return cb;
}

void reset_callbacks(void) {
    registry = 0;
}

CALLBACK register_callback(const char *name, void (*cbv)(va_list va))
{
    struct reg * r = (struct reg *)malloc(sizeof(struct reg));
    r->next = registry;
    r->name = name;
    r->cb.cbv = cbv;
    registry = r;
    return r->cb;
}

int find_callback(const char *name, CALLBACK *result) {
    if (result && name) {
        struct reg *r;
        for (r=registry;r;r=r->next) {
            if (strcmp(r->name, name)==0) {
                *result = r->cb;
                return 0;
            }
        }
    }
    return -1;
}

int call_callback(CALLBACK cb, const char *name, ... ) {
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
