#include "functions.h"

#include <critbit.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static critbit_tree cb_functions;

pf_generic get_function(const char *name)
{
    void * matches;
    pf_generic result;
    if (cb_find_prefix(&cb_functions, name, strlen(name) + 1, &matches, 1, 0)) {
        cb_get_kv(matches, &result, sizeof(result));
        return result;
    }
    return NULL;
}

void register_function(pf_generic fun, const char *name)
{
    char buffer[64];
    size_t len = strlen(name);

    assert(len < sizeof(buffer) - sizeof(fun));
    len = cb_new_kv(name, len, &fun, sizeof(fun), buffer);
    cb_insert(&cb_functions, buffer, len);
}

void free_functions(void) {
    cb_clear(&cb_functions);
}

