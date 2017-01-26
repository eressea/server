#pragma once

#define selist quicklist
#include <selist.h>

int ql_advance(struct quicklist **qlp, int *index, int stride);
void ql_foreach(struct quicklist *ql, selist_cb cb);
void ql_free(struct quicklist *ql);
int ql_delete(struct quicklist **qlp, int index);
void ql_push(struct quicklist **qlp, void *el);
int ql_length(const struct quicklist *ql);
void * ql_replace(struct quicklist *ql, int index, void *el);
void *ql_get(struct quicklist *ql, int index);
bool ql_empty(const struct quicklist *ql);
typedef void(*ql_cb)(void *);
bool ql_set_insert_ex(struct quicklist **qlp, void *data, int (*cmp_cb)(const void *lhs, const void *rhs));
bool ql_set_find_ex(struct quicklist **qlp, int *qip, const void *data, int (*cmp_cb)(const void *lhs, const void *rhs));

