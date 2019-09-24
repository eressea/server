#ifndef H_ATTRIBUTE_TARGETREGION
#define H_ATTRIBUTE_TARGETREGION
#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_targetregion;

    struct region;
    extern struct attrib *make_targetregion(struct region *);

#ifdef __cplusplus
}
#endif
#endif
