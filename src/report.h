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
    struct locale;
    void register_nr(void);
    void report_cleanup(void);
    void write_spaces(struct stream *out, size_t num);
    void report_travelthru(struct stream *out, struct region * r, const struct faction * f);
    void report_region(struct stream *out, const struct region * r, struct faction * f);

    void nr_spell_syntax(char *buf, size_t size, struct spellbook_entry * sbe, const struct locale *lang);
    void nr_spell(struct stream *out, struct spellbook_entry * sbe, const struct locale *lang);

#ifdef __cplusplus
}
#endif
#endif
