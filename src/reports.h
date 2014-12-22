/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

#include <time.h>
#include <kernel/objtypes.h>
#include <kernel/types.h>

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

    /* kann_finden speedups */
    bool kann_finden(struct faction *f1, struct faction *f2);
    struct unit *can_find(struct faction *, struct faction *);

    /* funktionen zum schreiben eines reports */
    void sparagraph(struct strlist **SP, const char *s, int indent, char mark);
    void lparagraph(struct strlist **SP, char *s, int indent, char mark);
    const char *hp_status(const struct unit *u);
    size_t spskill(char *pbuf, size_t siz, const struct locale *lang, const struct unit *u, struct skill *sv, int *dh, int days);  /* mapper */
    void spunit(struct strlist **SP, const struct faction *f,
        const struct unit *u, int indent, int mode);

    int reports(void);
    int write_reports(struct faction *f, time_t ltime);
    int init_reports(void);
    void reorder_units(struct region * r);

    const struct unit *ucansee(const struct faction *f,
        const struct unit *u, const struct unit *x);

    enum {
        see_none,
        see_neighbour,
        see_lighthouse,
        see_travel,
        see_far,
        see_unit,
        see_battle
    };
    int stealth_modifier(int seen_mode);

    typedef struct seen_region {
        struct seen_region *nextHash;
        struct seen_region *next;
        struct region *r;
        unsigned char mode;
        bool disbelieves;
    } seen_region;

    struct seen_region *find_seen(struct seen_region *seehash[],
        const struct region *r);
    bool add_seen(struct seen_region *seehash[], struct region *r,
        unsigned char mode, bool dis);
    struct seen_region **seen_init(void);
    void seen_done(struct seen_region *seehash[]);
    void free_seen(void);
    void link_seen(seen_region * seehash[], const struct region *first,
        const struct region *last);

    typedef struct report_context {
        struct faction *f;
        struct quicklist *addresses;
        struct seen_region **seen;
        struct region *first, *last;
        void *userdata;
        time_t report_time;
    } report_context;

    typedef int(*report_fun) (const char *filename, report_context * ctx,
        const char *charset);
    void register_reporttype(const char *extension, report_fun write,
        int flag);

    int bufunit(const struct faction *f, const struct unit *u, int indent,
        int mode, char *buf, size_t size);

    const char *trailinto(const struct region *r,
        const struct locale *lang);
    const char *report_kampfstatus(const struct unit *u,
        const struct locale *lang);

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
    int report_resources(const struct seen_region *sr,
    struct resource_report *result, int size, const struct faction *viewer);
    int report_items(const struct item *items, struct item *result, int size,
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

    typedef struct strlist {
        struct strlist *next;
        char *s;
    } strlist;

    void addstrlist(strlist ** SP, const char *s);
    void freestrlist(strlist * s);


#define GR_PLURAL     0x01      /* grammar: plural */
#define MAX_INVENTORY 128       /* maimum number of different items in an inventory */
#define MAX_RAWMATERIALS 8      /* maximum kinds of raw materials in a regions */

#ifdef __cplusplus
}
#endif
#endif
