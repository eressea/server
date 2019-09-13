#ifndef H_ATTRIBUTE_RACEPREFIX
#define H_ATTRIBUTE_RACEPREFIX
#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_raceprefix;
    extern void set_prefix(struct attrib **ap, const char *str);
    extern const char *get_prefix(struct attrib *a);

#ifdef __cplusplus
}
#endif
#endif
