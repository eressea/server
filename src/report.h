#ifndef H_GC_REPORT
#define H_GC_REPORT

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct stream;
    struct sbstring;
    struct spellbook_entry;
    struct region;
    struct faction;
    struct locale;
    struct ally;

    void register_nr(void);
    void report_cleanup(void);
    void write_spaces(struct stream *out, size_t num);
    void report_travelthru(struct stream *out, struct region * r, const struct faction * f);
    void report_region(struct stream *out, const struct region * r, struct faction * f);
    void report_allies(struct stream *out, size_t maxlen, const struct faction * f, struct ally * allies, const char *prefix);
    void pump_paragraph(struct sbstring *sbp, struct stream *out, size_t maxlen, bool isfinal);
    void paragraph(struct stream *out, const char *str, ptrdiff_t indent, int hanging_indent, char marker);

    void nr_spell_syntax(char *buf, size_t size, struct spellbook_entry * sbe, const struct locale *lang);
    void nr_spell(struct stream *out, struct spellbook_entry * sbe, const struct locale *lang);

    int write_template(const char* filename, const char* bom, const struct faction* f, const char* password,
        const struct region* regions_begin,
        const struct region* regions_end);
#ifdef __cplusplus
}
#endif
#endif
