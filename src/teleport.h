#ifndef TELEPORT_H
#define TELEPORT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TP_DISTANCE 4
#define TP_RADIUS (TP_DISTANCE/2) /* Radius von Schemen */
#define MAX_SCHEMES ((TP_RADIUS * 2 + 1) * (TP_RADIUS * 2 + 1) - 4)

    struct region;
    struct region_list;
    struct plane;

    struct region *r_standard_to_astral(const struct region *r);
    struct region *r_astral_to_standard(const struct region *r);
    bool inhabitable(const struct region *r);
    bool is_astral(const struct region *r);
    struct plane *get_astralplane(void);
    int get_astralregions(const struct region * r, bool(*valid) (const struct region *), struct region *result[]);
    int regions_in_range(const struct region * r, int radius, bool(*valid) (const struct region *), struct region *result[]);

    void create_teleport_plane(void);
    void spawn_braineaters(float chance);

    int real2tp(int rk);

#ifdef __cplusplus
}
#endif
#endif
