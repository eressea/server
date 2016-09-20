#pragma once
/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef H_KRNL_REPORTS
#define H_KRNL_REPORTS

#include <platform.h>
#include <time.h>
#include <kernel/objtypes.h>
#include <kernel/types.h>

struct stream;
struct seen_region;
#ifdef __cplusplus
extern "C" {
#endif

    /* Alter, ab dem der Score angezeigt werden soll: */
#define DISPLAYSCORE 12
    /* Breite einer Reportzeile: */
#define REPORTWIDTH 78

    extern const char *directions[];
    extern const char *coasts[];
    extern const char *combatstatus[];
    extern bool nonr;
    extern bool nocr;
    extern bool noreports;
    extern const char *visibility[];

    void reports_done(void);

    struct unit *can_find(struct faction *, struct faction *);

    bool omniscient(const struct faction *f);

    /* funktionen zum schreiben eines reports */
    void sparagraph(struct strlist **SP, const char *s, unsigned int indent, char mark);
    void lparagraph(struct strlist **SP, char *s, unsigned int indent, char mark);
    const char *hp_status(const struct unit *u);
    size_t spskill(char *pbuf, size_t siz, const struct locale *lang, const struct unit *u, struct skill *sv, int *dh, int days);  /* mapper */
    void spunit(struct strlist **SP, const struct faction *f,
        const struct unit *u, unsigned int indent, seen_mode mode);

    int reports(void);
    int write_reports(struct faction *f, time_t ltime);
    int init_reports(void);
    void reorder_units(struct region * r);

    const struct unit *ucansee(const struct faction *f,
        const struct unit *u, const struct unit *x);

    int stealth_modifier(seen_mode seen_mode);

    typedef struct report_context {
        struct faction *f;
        struct quicklist *addresses;
        struct region *first, *last;
        void *userdata;
        time_t report_time;
    } report_context;

    void prepare_report(report_context *ctx, struct faction *f);
    void finish_reports(report_context *ctx);

    typedef int(*report_fun) (const char *filename, report_context * ctx,
        const char *charset);
    void register_reporttype(const char *extension, report_fun write,
        int flag);

    int bufunit(const struct faction *f, const struct unit *u, unsigned int indent,
        seen_mode mode, char *buf, size_t size);

    const char *trailinto(const struct region *r,
        const struct locale *lang);
    size_t report_status(const struct unit *u,
        const struct locale *lang, char *buf, size_t siz);

    void register_reports(void);

    struct message *msg_curse(const struct curse *c, const void *obj,
        objtype_t typ, int slef);

    typedef struct arg_regions {
        int nregions;
        struct region **regions;
    } arg_regions;

    typedef struct resource_report {
        const char *name;
        int number;
        int level;
    } resource_report;
    int report_resources(const struct region *r, struct resource_report *res,
        int size, const struct faction *viewer, bool see_unit);
    int report_items(const struct unit *u, struct item *result, int size,
        const struct unit *owner, const struct faction *viewer);
    void report_item(const struct unit *owner, const struct item *i,
        const struct faction *viewer, const char **name, const char **basename,
        int *number, bool singular);
    void report_building(const struct building *b, const char **btype,
        const char **billusion);
    void report_race(const struct unit *u, const char **rcname,
        const char **rcillusion);

    void add_seen_faction(struct faction *self, struct faction *seen);
    size_t f_regionid(const struct region *r, const struct faction *f,
        char *buffer, size_t size);

    void split_paragraph(struct strlist ** SP, const char *s, unsigned int indent, unsigned int width, char mark);

    int stream_printf(struct stream * out, const char *format, ...);

    int count_travelthru(struct region *r, const struct faction *f);

#define GR_PLURAL     0x01      /* grammar: plural */
#define MAX_INVENTORY 128       /* maimum number of different items in an inventory */
#define MAX_RAWMATERIALS 8      /* maximum kinds of raw materials in a regions */

#ifdef __cplusplus
}
#endif
#endif
