#ifndef BIND_ERESSEA_CONFIG_H
#define BIND_ERESSEA_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

    void config_reset(void);
    int config_parse(const char *json);
    int config_read(const char *filename, const char * relpath);

#ifdef __cplusplus
}
#endif
#endif
