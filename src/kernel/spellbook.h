#ifndef H_KRNL_SPELLBOOK_H
#define H_KRNL_SPELLBOOK_H

#include "spell.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct storage;
    struct gamedata;
    struct selist;

    typedef struct spellbook_entry {
        spellref spref;
        int level;
    } spellbook_entry;

    typedef struct spellbook
    {
        char * name;
        struct spellbook_entry * spells; /* FIXME: selist only ever adds with push, could use stb array<spell *> */
    } spellbook;

    spellbook * create_spellbook(const char * name);

    void read_spellbook(struct spellbook **bookp, struct gamedata *data, int(*get_level)(const struct spell * sp, void *), void * cbdata);
    void write_spellbook(const struct spellbook *book, struct storage *store);

    void spellbook_add(spellbook *sbp, struct spell *sp, int level);
    void spellbook_addref(spellbook *sb, const char *name, int level);
    int spellbook_foreach(spellbook *sb, int(*callback)(spellbook_entry *, void *), void * data);
    void spellbook_clear(spellbook *sb);
    spellbook_entry * spellbook_get(spellbook *sb, const struct spell *sp);

#ifdef __cplusplus
}
#endif
#endif
