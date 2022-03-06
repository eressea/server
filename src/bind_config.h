#pragma once
#ifndef BIND_ERESSEA_CONFIG_H
#define BIND_ERESSEA_CONFIG_H

void config_reset(void);
int config_parse(const char *json);
int config_read(const char *filename, const char * relpath);

#endif
