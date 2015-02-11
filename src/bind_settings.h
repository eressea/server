#ifndef BIND_ERESSEA_SETTINGS_H
#define BIND_ERESSEA_SETTINGS_H
#ifdef __cplusplus
extern "C" {
#endif

    const char * settings_get(const char *key);
    void settings_set(const char *key, const char *value);

#ifdef __cplusplus
}
#endif
#endif
