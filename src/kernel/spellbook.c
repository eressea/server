#include <platform.h>
#include <kernel/config.h>
#include <util/quicklist.h>

#include "spellbook.h"

void spellbook_add(spellbook **sbp, struct spell * sp, int level)
{
  spellbook_entry * sbe = (spellbook_entry *)malloc(sizeof(spellbook_entry));
  sbe->sp = sp;
  sbe->level = level;
  ql_push(sbp, sbe);
}

void spellbook_free(spellbook *sb)
{
  quicklist *ql;
  int qi;

  for (qi = 0, ql = sb; ql; ql_advance(&ql, &qi, 1)) {
    spellbook_entry *sbe = (spellbook_entry *) ql_get(ql, qi);
    free(sbe);
  }
  ql_free(sb);
}

int spellbook_foreach(spellbook *sb, int (*callback)(spellbook_entry *, void *), void * data)
{
  quicklist *ql;
  int qi;

  for (qi = 0, ql = sb; ql; ql_advance(&ql, &qi, 1)) {
    spellbook_entry *sbe = (spellbook_entry *) ql_get(ql, qi);
    int result = callback(sbe, data);
    if (result) {
      return result;
    }
  }
  return 0;
}
