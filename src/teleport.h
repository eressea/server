#ifndef TELEPORT_H
#define TELEPORT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct region;
    struct region_list;
    struct plane;

    struct region *r_standard_to_astral(const struct region *r);
    struct region *r_astral_to_standard(const struct region *);
    struct region_list *astralregions(const struct region *rastral,
        bool(*valid) (const struct region *));
    struct region_list *all_in_range(const struct region *r, int n,
        bool(*valid) (const struct region *));
    bool inhabitable(const struct region *r);
    bool is_astral(const struct region *r);
    struct plane *get_astralplane(void);

    void create_teleport_plane(void);
    void spawn_braineaters(float chance);

    int real2tp(int rk);

#ifdef __cplusplus
}
#endif
#endif
