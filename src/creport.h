/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */
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
        const struct region *r, bool see_unit);
#ifdef __cplusplus
}
#endif
#endif
