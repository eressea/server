#pragma once
#ifndef H_KRNL_PATHFINDER
#define H_KRNL_PATHFINDER

struct region;

struct region **path_find(struct region *handle_start,
    const struct region *target, int maxlen,
    bool(*allowed) (const struct region *, const struct region *));
bool path_exists(struct region *handle_start, const struct region *target,
    int maxlen, bool(*allowed) (const struct region *, const struct region *));
bool allowed_fly(const struct region *src, const struct region *target);
bool allowed_walk(const struct region *src, const struct region *target);

struct region **path_regions_in_range(struct region *src, int maxdist,
    bool(*allowed) (const struct region *, const struct region *));

void pathfinder_cleanup(void);

#endif
