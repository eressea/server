#include <platform.h>

#include <selist.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "resolve.h"
#include "storage.h"
#include "variant.h"

typedef struct unresolved {
    int id;
    /* information on how to resolve the missing object */
    resolve_fun resolve;
    /* function to resolve the unknown object */
    selist *addrs;
    /* address to pass to the resolve-function */
} unresolved;

#define HASHSIZE 1024 /* must be a power of 2 */
static unresolved ur_hash[HASHSIZE];

int ur_key(int id) {
    int h = id ^ (id >> 16);
    return h & (HASHSIZE - 1);
}

void ur_add(int id, void **addr, resolve_fun fun)
{
    int h, i;

    assert(id > 0);
    assert(addr);
    assert(!*addr);
    
    h = ur_key(id);
    for (i = 0; i != HASHSIZE; ++i) {
        int k = h + i;
        if (k >= HASHSIZE) k -= HASHSIZE;
        if (ur_hash[k].id <= 0) {
            ur_hash[k].id = id;
            ur_hash[k].resolve = fun;
            selist_push(&ur_hash[k].addrs, addr);
            return;
        }
        if (ur_hash[k].id == id && ur_hash[k].resolve == fun) {
            ur_hash[k].resolve = fun;
            selist_push(&ur_hash[k].addrs, addr);
            return;
        }
    }
    assert(!"hash table is full");
}

static bool addr_cb(void *data, void *more) {
    void **addr = (void **)data;
    *addr = more;
    return true;
}

void resolve(int id, void *data)
{
    int h, i;
    h = ur_key(id);

    for (i = 0; i != HASHSIZE; ++i) {
        int k = h + i;

        if (k >= HASHSIZE) k -= HASHSIZE;
        if (ur_hash[k].id == 0) break;
        else if (ur_hash[k].id == id) {
            if (ur_hash[k].resolve) {
                data = ur_hash[k].resolve(id, data);
            }
            selist_foreach_ex(ur_hash[k].addrs, addr_cb, data);
            selist_free(ur_hash[k].addrs);
            ur_hash[k].addrs = NULL;
            ur_hash[k].id = -1;
        }
    }
}
