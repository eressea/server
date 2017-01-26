#include "quicklist.h"
#include <stdlib.h>

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
