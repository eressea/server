#pragma once

#include <stddef.h>

typedef unsigned int dbstring_id;

void swapdb_open(void);
void swapdb_close(void);

dbstring_id dbstring_save(const char *s);
const char *dbstring_load(dbstring_id id, size_t *size);
