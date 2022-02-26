#ifndef H_KERNEL_GROUP
#define H_KERNEL_GROUP
#ifdef __cplusplus
extern "C" {
#endif

    struct gamedata;
    struct unit;
    struct faction;

    typedef struct group {
        struct group *next;
        struct group *nexthash;
        struct faction *f;
        struct attrib *attribs;
        char *name;
        struct allies *allies;
        int gid;
        int members;
    } group;

    extern struct attrib_type at_group;   /* attribute for units assigned to a group */
    extern struct group *join_group(struct unit *u, const char *name);
    extern void set_group(struct unit *u, struct group *g);
    extern struct group * get_group(const struct unit *u);
    extern void free_group(struct group *g);
    struct group *create_group(struct faction * f, const char *name, int gid);

    extern void write_groups(struct gamedata *data, const struct faction *f);
    extern void read_groups(struct gamedata *data, struct faction *f);

#ifdef __cplusplus
}
#endif
#endif
