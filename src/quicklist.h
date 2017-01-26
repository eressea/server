#pragma once

#define selist quicklist
#define selist_free ql_free
#define selist_delete ql_delete
#define selist_foreach ql_foreach
#define selist_get ql_get
#define selist_replace ql_replace
#define selist_length ql_length
#define selist_push ql_push
#define selist_empty ql_empty
#define selist_advance ql_advance
#include <selist.h>
#include <stdbool.h>

typedef void(*ql_cb)(void *);
bool ql_set_insert_ex(struct quicklist **qlp, void *data, int (*cmp_cb)(const void *lhs, const void *rhs));
bool ql_set_find_ex(struct quicklist **qlp, int *qip, const void *data, int (*cmp_cb)(const void *lhs, const void *rhs));

