#include "quicklist.h"
#include <stdlib.h>

void ql_push(struct quicklist **qlp, void *el)
{
    selist_push(qlp, el);
}

int ql_length(const struct quicklist *ql)
{
    return selist_length(ql);
}

void * ql_replace(struct quicklist *ql, int index, void *el)
{
    return selist_replace(ql, index, el);
}

void *ql_get(struct quicklist *ql, int index)
{ 
    return selist_get(ql, index);
}

int ql_delete(struct quicklist **qlp, int index)
{
    return selist_delete(qlp, index);
}

bool ql_empty(const struct quicklist *ql)
{
    return selist_empty(ql);
}

void ql_foreach(struct quicklist *ql, selist_cb cb)
{
    selist_foreach(ql, cb);
}

int ql_advance(struct quicklist **qlp, int *index, int stride)
{
    return selist_advance(qlp, index, stride);
}

void ql_free(struct quicklist *ql)
{
    selist_free(ql);
}

bool ql_set_remove(struct quicklist **qlp, const void *data)
{
    int qi, qn;
    quicklist *ql = *qlp;

    if (!ql)
        return false;

    qn = selist_length(ql);
    for (qi = 0; qi != qn; ++qi) {
        void *qd = selist_get(ql, qi);
        if (qd == data) {
            return selist_delete(qlp, qi) == 0;
        }
    }
    return false;
}

bool ql_set_insert(struct quicklist **qlp, void *data)
{
    return selist_set_insert(qlp, data, NULL);
}

bool ql_set_find(struct quicklist **qlp, int *qip, const void *data)
{
    return selist_set_find(qlp, qip, data, NULL);
}
