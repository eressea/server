#ifndef H_ATTRIBUTE_FOLLOW
#define H_ATTRIBUTE_FOLLOW
#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_follow;

    struct unit;

    struct attrib *make_follow(struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
