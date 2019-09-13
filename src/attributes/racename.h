#ifndef H_ATTRIBUTE_RACENAME_H
#define H_ATTRIBUTE_RACENAME_H
#ifdef __cplusplus
extern "C" {
#endif

    struct attrib_type;
    struct attrib;

    extern void set_racename(struct attrib **palist, const char *name);
    extern const char *get_racename(struct attrib *alist);

    extern struct attrib_type at_racename;

#ifdef __cplusplus
}
#endif
#endif
