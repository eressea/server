#ifndef H_GC_CREPORT
#define H_GC_CREPORT

#include <kernel/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct stream;
    struct region;
    struct faction;
    struct unit;

    void creport_cleanup(void);
    void register_cr(void);

    int crwritemap(const char *filename);
    void cr_output_unit(struct stream *out, const struct faction * f,
        const struct unit * u, seen_mode mode);
    void cr_output_resources(struct stream *out, const struct faction * f,
        const struct region *r, seen_mode mode);
#ifdef __cplusplus
}
#endif
#endif
