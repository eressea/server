/*
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2007   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

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
