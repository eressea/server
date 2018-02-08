#ifndef H_SEENSPELL
#define H_SEENSPELL

struct attrib_type;
struct spellbook;
struct faction;

void show_new_spells(struct faction * f, int level, const struct spellbook *book);

extern struct attrib_type at_reportspell;
extern struct attrib_type at_seenspell;

#endif
