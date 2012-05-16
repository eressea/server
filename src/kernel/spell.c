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

#include <platform.h>
#include <kernel/config.h>
#include "spell.h"

/* kernel includes */
#include "magic.h"
#include "unit.h"

/* libc includes */
#include <assert.h>
#include <string.h>

/* util includes */
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/umlaut.h>
#include <util/quicklist.h>

quicklist *spells = NULL;

spell * create_spell(const char * name)
{
  spell * sp = (spell *) calloc(1, sizeof(spell));
  sp->sname = strdup(name);
  return sp;
}

void register_spell(spell * sp)
{
  if (sp->id == 0) {
    sp->id = hashstring(sp->sname);
  }
  add_spell(&spells, sp);
}

spell *find_spell(const char *name)
{
  quicklist *ql = spells;
  int qi;

  for (qi = 0; ql; ql_advance(&ql, &qi, 1)) {
    spell *sp = (spell *) ql_get(ql, qi);
    if (strcmp(name, sp->sname) == 0) {
      return sp;
    }
  }
  log_warning("find_spell: could not find spell '%s'\n", name);
  return 0;
}

/* ------------------------------------------------------------- */
/* Spruch identifizieren */

spell *get_spellfromtoken(sc_mage *mage, const char *name,
  const struct locale * lang)
{
  variant token;
  struct spell_names * names = mage->spellnames;
  for (;names;names=names->next) {
    if (names->lang==lang) break;
  }
  if (!names) {
    quicklist *ql = mage->spells;
    int qi;
    names = (spell_names *)calloc(1, sizeof(spell_names));
    names->next = mage->spellnames;
    names->lang = lang;
    names->tokens = 0;
    for (qi = 0, ql = mage->spells; ql; ql_advance(&ql, &qi, 1)) {
      spell *sp = (spell *) ql_get(ql, qi);
      const char *n = spell_name(sp, lang);
      token.v = sp;
      addtoken(&names->tokens, n, token);
    }
    mage->spellnames = names;
  }

  if (findtoken(names->tokens, name, &token) != E_TOK_NOMATCH) {
    return (spell *) token.v;
  }
  return 0;
}

spell *find_spellbyid(unsigned int id)
{
  quicklist *ql;
  int qi;

  if (id == 0)
    return NULL;
  for (qi = 0, ql = spells; ql; ql_advance(&ql, &qi, 1)) {
    spell *sp = (spell *) ql_get(ql, qi);
    if (sp->id == id) {
      return sp;
    }
  }
  for (qi = 0, ql = spells; ql; ql_advance(&ql, &qi, 1)) {
    spell *sp = (spell *) ql_get(ql, qi);
    unsigned int hashid = hashstring(sp->sname);
    if (hashid == id) {
      return sp;
    }
  }

  log_warning("cannot find spell by id: %u\n", id);
  return NULL;
}
