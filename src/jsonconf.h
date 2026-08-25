#pragma once

struct cJSON;

extern const char * json_relpath;

void add_authority(const char *key, const char *path);

void json_config(struct cJSON *str);
void jsonconf_done(void);
