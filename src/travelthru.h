#pragma once

#ifndef H_TRAVELTHRU
#define H_TRAVELTHRU
#ifdef __cplusplus
extern "C" {
#endif

    extern struct attrib_type at_travelunit;

    struct stream;
    struct region;
    struct faction;
    void write_travelthru(struct stream *out, const struct region * r, const struct faction * f);
    void travelthru(const struct unit * u, struct region * r);

#ifdef __cplusplus
}
#endif
#endif
