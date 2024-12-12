#pragma once

#include <stddef.h>

int check_email(const char *newmail);

int *intlist_init(void);
int *intlist_add(int *i_p, int i);
int *intlist_find(int *i_p, int i);

void scramble_array(void *data, size_t n, size_t width);
