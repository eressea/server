#ifndef H_SEENSPELL
#define H_SEENSPELL

struct attrib_type;
struct spellbook_entry;
struct faction;
struct spell;

void show_spell(struct faction * f, const struct spellbook_entry *sbe);
void reset_seen_spells(struct faction * f, const struct spell *sp);

extern struct attrib_type at_reportspell;
extern struct attrib_type at_seenspells;
extern struct attrib_type at_seenspell; /* upgraded */

#endif
