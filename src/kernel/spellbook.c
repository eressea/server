#include <platform.h>
#include <kernel/config.h>
#include <kernel/spell.h>
#include <kernel/magic.h>
#include <util/quicklist.h>

#include "spellbook.h"

#include <assert.h>

spellbook * create_spellbook(const char * name)
{
  spellbook *result = (spellbook *)malloc(sizeof(spellbook));
  result->name = name ? strdup(name) : 0;
  result->spells = 0;
  return result;
}

void spellbook_add(spellbook *sb, struct spell * sp, int level)
{
  spellbook_entry * sbe = (spellbook_entry *)malloc(sizeof(spellbook_entry));

  assert(sb);
  sbe->sp = sp;
  sbe->level = level;
  ql_push(&sb->spells, sbe);
}

void spellbook_clear(spellbook *sb)
{
  quicklist *ql;
  int qi;

  assert(sb);
  for (qi = 0, ql = sb->spells; ql; ql_advance(&ql, &qi, 1)) {
    spellbook_entry *sbe = (spellbook_entry *) ql_get(ql, qi);
    free(sbe);
  }
  ql_free(sb->spells);
}

int spellbook_foreach(spellbook *sb, int (*callback)(spellbook_entry *, void *), void * data)
{
  quicklist *ql;
  int qi;

  for (qi = 0, ql = sb->spells; ql; ql_advance(&ql, &qi, 1)) {
    spellbook_entry *sbe = (spellbook_entry *) ql_get(ql, qi);
    int result = callback(sbe, data);
    if (result) {
      return result;
    }
  }
  return 0;
}

spellbook_entry * spellbook_get(spellbook *sb, const struct spell * sp)
{
  if (sb) {
    quicklist *ql;
    int qi;

    for (qi = 0, ql = sb->spells; ql; ql_advance(&ql, &qi, 1)) {
      spellbook_entry *sbe = (spellbook_entry *) ql_get(ql, qi);
      if (sp==sbe->sp) {
        return sbe;
      }
    }
  }
  return 0;
}

