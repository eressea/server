#ifndef H_ATTRIBUTE_KEY
#define H_ATTRIBUTE_KEY

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
    struct attrib;
    struct attrib_type;
    extern struct attrib_type at_key; /* DEPRECATED: at_key has been replaced with at_keys */
    extern struct attrib_type at_keys;

    void key_set(struct attrib **alist, int key, int value);
    void key_unset(struct attrib **alist, int key);
    int key_get(struct attrib *alist, int key);

#ifdef __cplusplus
}
#endif
#endif
