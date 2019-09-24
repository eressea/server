#ifndef H_ATTRIBUTE_OBJECT
#define H_ATTRIBUTE_OBJECT

struct attrib_type;
struct attrib;

#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_dict; /* DEPRECATED: at_dict has been replaced with at_keys */

    void dict_set(struct attrib * a, const char * name, int value);

#ifdef __cplusplus
}
#endif
#endif
