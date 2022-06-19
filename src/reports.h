#ifndef H_KRNL_REPORTS
#define H_KRNL_REPORTS

#include <time.h>
#include <kernel/objtypes.h>
#include <kernel/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct battle;
    struct gamedate;
    struct sbstring;
    struct selist;
    struct stream;
    struct seen_region;
    struct region;
    enum seen_mode;

    /* Alter, ab dem der Score angezeigt werden soll: */
#define DISPLAYSCORE 12
    /* Breite einer Reportzeile: */
#define REPORTWIDTH 78

#define GR_PLURAL     0x01      /* grammar: plural */
#define MAX_INVENTORY 128       /* maimum number of different items in an inventory */
#define MAX_RAWMATERIALS 12     /* maximum kinds of resources in a regions */


    extern const char *directions[];
    extern const char *coasts[];
    extern const char *combatstatus[];
    extern bool nonr;
    extern bool nocr;
    extern bool noreports;
    extern const char *visibility[];
    extern const char *options[MAXOPTIONS];    /* report options */

    void reports_done(void);

    bool omniscient(const struct faction *f);
    struct selist *get_regions_distance(struct region * root, int radius);
    int get_regions_distance_arr(struct region *r, int radius, struct region *result[], int size);
    /* funktionen zum schreiben eines reports */
    void sparagraph(struct strlist **SP, const char *s, unsigned int indent, char mark);
    const char *hp_status(const struct unit *u);

    int reports(void);
    int write_reports(struct faction *f, int options, const char *password);
    int init_reports(void);

    int stealth_modifier(const struct region *r, const struct faction *f, enum seen_mode mode);

    typedef struct report_context {
        struct faction *f;
        struct selist *addresses;
        struct region *first, *last;
        void *userdata;
        time_t report_time;
        const char *password;
    } report_context;

    void update_defaults(struct faction* f);
    void prepare_report(report_context *ctx, struct faction *f, const char *password);
    void finish_reports(report_context *ctx);
    void get_addresses(report_context * ctx);

    typedef int(*report_fun) (const char *filename, report_context * ctx,
        const char *charset);
    void register_reporttype(const char *extension, report_fun write,
        int flag);

    int bufunit_depr(const struct faction *f, const struct unit *u, enum seen_mode mode,
        char *buf, size_t size);
    void bufunit(const struct faction * f, const struct unit * u,
        const struct faction *fv, enum seen_mode mode, bool getarnt,
        struct sbstring *sbp);

    const char *trailinto(const struct region *r,
        const struct locale *lang);
    void report_battle_start(struct battle * b);

    void register_reports(void);

    struct message *msg_curse(const struct curse *c, const void *obj,
        objtype_t typ, int slef);

    typedef struct arg_regions {
        int nregions;
        struct region **regions;
    } arg_regions;

    typedef struct resource_report {
        const struct resource_type *rtype;
        int number;
        int level;
    } resource_report;
    int report_resources(const struct region *r, struct resource_report res[MAX_RAWMATERIALS],
        const struct faction *viewer, enum seen_mode mode);
    int report_items(const struct unit *u, struct item *result, int size,
        const struct unit *owner, const struct faction *viewer);
    void report_warnings(struct faction *f, int now);
    void report_status(const struct unit * u, const struct locale *lang, struct sbstring *sbp);
    void report_raceinfo(const struct race *rc, const struct locale *lang, struct sbstring *sbp);
    void report_race_skills(const struct race *rc, const struct locale *lang, struct sbstring *sbp);
    void report_item(const struct unit *owner, const struct item *i,
        const struct faction *viewer, const char **name, const char **basename,
        int *number, bool singular);
    void report_building(const struct building *b, const char **btype,
        const char **billusion);
    void add_seen_faction(struct faction *self, struct faction *seen);
    size_t f_regionid(const struct region *r, const struct faction *f,
        char *buffer, size_t size);

    void split_paragraph(struct strlist ** SP, const char *s, unsigned int indent, unsigned int width, char mark);

    int stream_printf(struct stream * out, const char *format, ...);

    int count_travelthru(struct region *r, const struct faction *f);
    const char *get_mailcmd(const struct locale *loc);

    bool visible_unit(const struct unit *u, const struct faction *f, int stealthmod, enum seen_mode mode);

    bool see_schemes(const struct region *r);

#ifdef __cplusplus
}
#endif
#endif
