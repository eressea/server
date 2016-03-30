#pragma once
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
#ifndef H_GC_REPORT
#define H_GC_REPORT

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct stream;
    struct spellbook_entry;
    struct region;
    struct faction;
    struct seen_region;

    void register_nr(void);
    void report_cleanup(void);
    void write_spaces(struct stream *out, size_t num);
    void write_travelthru(struct stream *out, const struct region * r, const struct faction * f);

    void nr_spell_syntax(struct stream *out, struct spellbook_entry * sbe, const struct locale *lang);
    void nr_spell(struct stream *out, struct spellbook_entry * sbe, const struct locale *lang);

    void nr_ship(struct stream *out, const struct region * r, const struct ship * sh, const struct faction * f,
    const struct unit * captain);
    void nr_units(struct stream *out, const struct seen_region *sr, const struct faction *f);

#ifdef __cplusplus
}
#endif
#endif
