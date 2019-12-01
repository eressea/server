#ifndef H_ATTRIBUTE_ATTRIBUTES
#define H_ATTRIBUTE_ATTRIBUTES
#ifdef __cplusplus
extern "C" {
#endif

    struct attrib_type;
    struct region;
    struct faction;

    extern void register_attributes(void);

    void set_observer(struct region *r, struct faction *f, int perception, int turns);
    int get_observer(const struct region *r, const struct faction *f);

#ifdef __cplusplus
}
#endif
#endif
