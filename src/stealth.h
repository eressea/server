#ifndef STEALTH_H
#define STEALTH_H

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct region;
    extern struct attrib_type at_stealth;
    int eff_stealth(const struct unit *u, const struct region *r);
    void u_seteffstealth(struct unit *u, int value);
    int u_geteffstealth(const struct unit *u);

#ifdef __cplusplus
}
#endif

#endif
