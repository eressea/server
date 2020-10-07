#ifndef LIGHTHOUSE_H
#define LIGHTHOUSE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


    struct faction;
    struct region;
    struct building;
    struct building_type;
    struct unit;
    struct attrib;

    extern struct attrib_type at_lighthouse;
    /* leuchtturm */
    bool is_lighthouse(const struct building_type *btype);
    bool lighthouse_guarded(const struct region *r);
    void update_lighthouse(struct building *b);
    bool update_lighthouses(struct region *r);
    int lighthouse_range(const struct building *b);
    int lighthouse_view_distance(const struct building *b, const struct unit *u);


#ifdef __cplusplus
}
#endif
#endif
