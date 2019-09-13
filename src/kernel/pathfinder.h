#ifndef H_KRNL_PATHFINDER
#define H_KRNL_PATHFINDER
#ifdef __cplusplus
extern "C" {
#endif

    extern struct region **path_find(struct region *handle_start,
        const struct region *target, int maxlen,
        bool(*allowed) (const struct region *, const struct region *));
    extern bool path_exists(struct region *handle_start, const struct region *target,
        int maxlen, bool(*allowed) (const struct region *,
        const struct region *));
    extern bool allowed_swim(const struct region *src,
        const struct region *target);
    extern bool allowed_fly(const struct region *src,
        const struct region *target);
    extern bool allowed_walk(const struct region *src,
        const struct region *target);
    extern struct selist *regions_in_range(struct region *src, int maxdist,
        bool(*allowed) (const struct region *, const struct region *));

    extern void pathfinder_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif
