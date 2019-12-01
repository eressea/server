#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct faction;
    struct attrib;
    extern struct attrib_type at_otherfaction;

    struct faction *get_otherfaction(const struct unit *u);
    struct attrib *make_otherfaction(struct faction *f);
    struct faction *visible_faction(const struct faction *f,
        const struct unit *u);

#ifdef __cplusplus
}
#endif
