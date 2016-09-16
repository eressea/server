#pragma once

#ifndef H_TRAVELTHRU
#define H_TRAVELTHRU
#ifdef __cplusplus
extern "C" {
#endif

    struct attrib;
    struct stream;
    struct region;
    struct faction;
    struct unit;
    void travelthru_map(const struct region * r, void(*cb)(const struct region *r, struct unit *, void *), void *cbdata);
    bool travelthru_cansee(const struct region *r, const struct faction *f, const struct unit *u);
    void travelthru_add(struct region * r, struct unit * u);

#ifdef __cplusplus
}
#endif
#endif
