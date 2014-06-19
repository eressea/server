#ifndef H_BIND_LOCALE_H
#define H_BIND_LOCALE_H
#ifdef __cplusplus
extern "C" {
#endif

    void locale_create(const char *lang);
    void locale_set(const char *lang, const char *key, const char *str);
    const char * locale_get(const char *lang, const char *key);

#ifdef __cplusplus
}
#endif

#endif
