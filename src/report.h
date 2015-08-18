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
    struct region;
    struct faction;
    void register_nr(void);
    void report_cleanup(void);
    void write_spaces(struct stream *out, size_t num);

    void paragraph(struct stream *out, const char *str, ptrdiff_t indent, int hanging_indent, char marker);
#ifdef __cplusplus
}
#endif
#endif
