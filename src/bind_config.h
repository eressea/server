#ifndef BIND_ERESSEA_CONFIG_H
#define BIND_ERESSEA_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

void config_parse(const char *json);
void config_read(const char *filename);

#ifdef __cplusplus
}
#endif
#endif
