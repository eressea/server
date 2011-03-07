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

 /* Bitte die Sprüche nach Gebieten und Stufe ordnen, denn in derselben
  * Reihenfolge wie in Spelldaten tauchen sie auch im Report auf
  */

quicklist *spells = NULL;

void register_spell(spell * sp)
{
  if (sp->id == 0) {
    sp->id = hashstring(sp->sname);
  }
  add_spell(&spells, sp);
}

/** versucht einen Spruch über gebiet + name zu identifizieren.
 * gibt ansonsten NULL zurück */
spell *find_spell(magic_t mtype, const char *name)
{
  quicklist *ql = spells;

  int qi;

  spell *spx = NULL;

  for (qi = 0; ql; ql_advance(&ql, &qi, 1)) {
    spell *sp = (spell *) ql_get(ql, qi);

    if (strcmp(name, sp->sname) == 0) {
      if (mtype == M_NONE || sp->magietyp == mtype)
        return sp;
      spx = sp;
    }
  }
  if (spx == NULL) {
    log_error(("cannot find spell by name: %s\n", name));
  }
  return spx;
}


/* ------------------------------------------------------------- */
/* Spruch identifizieren */

typedef struct spell_names {
  struct spell_names *next;
  const struct locale *lang;
  magic_t mtype;
  struct tnode names;
} spell_names;

static spell_names *spellnames;

static spell_names *init_spellnames(const struct locale *lang, magic_t mtype)
{
  quicklist *ql;

  int qi;

  spell_names *sn = (spell_names *) calloc(sizeof(spell_names), 1);

  sn->next = spellnames;
  sn->lang = lang;
  sn->mtype = mtype;
  for (qi = 0, ql = spells; ql; ql_advance(&ql, &qi, 1)) {
    spell *sp = (spell *) ql_get(ql, qi);

    if (sp->magietyp == mtype) {
      const char *n = spell_name(sp, lang);

      variant token;

      token.v = sp;
      addtoken(&sn->names, n, token);
    }
  }
  return spellnames = sn;
}

static spell_names *get_spellnames(const struct locale *lang, magic_t mtype)
{
  spell_names *sn = spellnames;

  while (sn) {
    if (sn->mtype == mtype && sn->lang == lang)
      break;
    sn = sn->next;
  }
  if (!sn)
    return init_spellnames(lang, mtype);
  return sn;
}

static spell *get_spellfromtoken_i(const char *name, const struct locale *lang,
  magic_t mtype)
{
  variant token = { 0 };
  spell_names *sn;

  sn = get_spellnames(lang, mtype);
  if (findtoken(&sn->names, name, &token) == E_TOK_NOMATCH) {
    magic_t mt;

    /* if we could not find it in the main magic type, we look through all the others */
    for (mt = 0; mt != MAXMAGIETYP; ++mt) {
      if (mt != mtype) {
        sn = get_spellnames(lang, mt);
        if (findtoken(&sn->names, name, &token) != E_TOK_NOMATCH)
          break;
      }
    }
  }

  if (token.v != NULL)
    return (spell *) token.v;
  if (lang == default_locale)
    return NULL;
  return get_spellfromtoken_i(name, default_locale, mtype);
}

spell *get_spellfromtoken(unit * u, const char *name,
  const struct locale * lang)
{
  sc_mage *m = get_mage(u);

  spell *sp;

  if (m == NULL)
    return NULL;
  sp = get_spellfromtoken_i(name, lang, m->magietyp);
  if (sp != NULL) {
    quicklist *ql = m->spells;

    int qi;

    if (ql_set_find(&ql, &qi, sp)) {
      return sp;
    }
  }
  return NULL;
}

spell *find_spellbyid(magic_t mtype, spellid_t id)
{
  quicklist *ql;

  int qi;

  assert(id >= 0);
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
      if (sp->magietyp == mtype || mtype == M_NONE) {
        return sp;
      }
    }
  }

  log_warning(("cannot find spell by id: %u\n", id));
  return NULL;
}
