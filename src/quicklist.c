#include "quicklist.h"

bool ql_set_insert_ex(struct quicklist **qlp, void *data, int (*cmp_cb)(const void *lhs, const void *rhs));
bool ql_set_find_ex(struct quicklist **qlp, int *qip, const void *data, int (*cmp_cb)(const void *lhs, const void *rhs));
