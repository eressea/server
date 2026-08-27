#pragma once

#include <stdbool.h>

struct cJSON;

extern const char * json_relpath;

void add_authority(const char *key, const char *path);

bool json_config(struct cJSON *str);
void jsonconf_done(void);
