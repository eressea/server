#pragma once

#ifndef H_VORTEX
#define H_VORTEX

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct region;
    struct attrib;
    struct locale;

    typedef struct spec_direction {
        int x, y;
        int duration;
        bool active;
        char *desc;
        char *keyword;
    } spec_direction;

    extern struct attrib_type at_direction;

    struct region *find_special_direction(const struct region *r,
        const char *token);
    void register_special_direction(struct locale *lang, const char *name);
    struct spec_direction *special_direction(const struct region * from,
        const struct region * to);
    struct attrib *create_special_direction(struct region *r, struct region *rt,
        int duration, const char *desc, const char *keyword, bool active);

#ifdef __cplusplus
}
#endif
#endif
