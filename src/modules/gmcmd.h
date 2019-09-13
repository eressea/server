#ifndef H_MOD_GMCMD
#define H_MOD_GMCMD
#ifdef __cplusplus
extern "C" {
#endif

    struct plane;
    struct attrib;
    struct unit;
    struct faction;
    struct region;

    extern void register_gmcmd(void);
    /* initialize this module */

    extern void gmcommands(void);
    /* execute commands */

    extern struct faction *gm_addfaction(const char *email, struct plane *p,
    struct region *r);
    extern struct plane *gm_addplane(int radius, unsigned int flags,
        const char *name);

    /*
     * doesn't belong in here:
     */
    struct attrib *find_key(struct attrib *attribs, int key);

#ifdef __cplusplus
}
#endif
#endif
