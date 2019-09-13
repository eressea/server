#ifndef H_ATTRIBUTE_MOVEMENT
#define H_ATTRIBUTE_MOVEMENT
#ifdef __cplusplus
extern "C" {
#endif

    bool get_movement(struct attrib *const *alist, int type);
    void set_movement(struct attrib **alist, int type);

    extern struct attrib_type at_movement;
    extern struct attrib_type at_speedup;

#ifdef __cplusplus
}
#endif
#endif
