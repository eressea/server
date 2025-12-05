#pragma once

void bind_config_reset(void);
int bind_config_parse(const char *json);
int bind_config_read(const char *filename, const char * relpath);
const char *bind_config_get(const char *key);
