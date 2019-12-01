#ifndef H_JSONCONF_H
#define H_JSONCONF_H
#ifdef __cplusplus
extern "C" {
#endif

    struct cJSON;

    extern const char * json_relpath;

    void json_config(struct cJSON *str);

#ifdef __cplusplus
}
#endif
#endif
