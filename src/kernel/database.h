#pragma once

#include "db/driver.h"

#include <stddef.h>

void swapdb_open(void);
void swapdb_close(void);

dbrow_id dbstring_save(const char *s);
const char *dbstring_load(dbrow_id id, size_t *size);
