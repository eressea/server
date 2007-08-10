/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7		    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#include "spell.h"

/* kernel includes */
#include "magic.h"
#include "spellid.h"
#include "unit.h"

/* libc includes */
#include <assert.h>
#include <string.h>

/* util includes */
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/umlaut.h>

 /* Bitte die Sprüche nach Gebieten und Stufe ordnen, denn in derselben
  * Reihenfolge wie in Spelldaten tauchen sie auch im Report auf
  */

spell_list * spells = NULL;

void
register_spell(spell * sp)
{
  if (sp->id==0) {
    sp->id = hashstring(sp->sname);
  }
  spelllist_add(&spells, sp);
}

/** versucht einen Spruch über gebiet + name zu identifizieren.
 * gibt ansonsten NULL zurück */
spell * 
find_spell(magic_t mtype, const char * name)
{
  spell_list * slist = spells;
  spell * spx = NULL;
  while (slist) {
    spell * sp = slist->data;
    if (strcmp(name, sp->sname)==0) {
      if (sp->magietyp==mtype) return sp;
      spx = sp;
    }
    slist = slist->next;
  }
  if (spx==NULL) {
    log_error(("cannot find spell by name: %s\n", name));
  }
  return spx;
}


/* ------------------------------------------------------------- */
/* Spruch identifizieren */

typedef struct spell_names {
  struct spell_names * next;
  const struct locale * lang;
  magic_t mtype;
  struct tnode names;
} spell_names;

static spell_names * spellnames;

static spell_names *
init_spellnames(const struct locale * lang, magic_t mtype)
{
  spell_list * slist;
  spell_names * sn = calloc(sizeof(spell_names), 1);
  sn->next = spellnames;
  sn->lang = lang;
  sn->mtype = mtype;
  for (slist=spells;slist!=NULL;slist=slist->next) {
    spell * sp = slist->data;
    if (sp->magietyp==mtype) {
      const char * n = spell_name(sp, lang);
      variant token;
      token.v = sp;
      addtoken(&sn->names, n, token);
    }
  }
  return spellnames = sn;
}

static spell_names *
get_spellnames(const struct locale * lang, magic_t mtype)
{
  spell_names * sn = spellnames;
  while (sn) {
    if (sn->mtype==mtype && sn->lang==lang) break;
    sn=sn->next;
  }
  if (!sn) return init_spellnames(lang, mtype);
  return sn;
}

static spell *
get_spellfromtoken_i(const char *name, const struct locale * lang, magic_t mtype)
{
  variant token = { 0 };
  spell_names * sn;

  sn = get_spellnames(lang, mtype);
  if (findtoken(&sn->names, name, &token)==E_TOK_NOMATCH) {
    magic_t mt;
    /* if we could not find it in the main magic type, we look through all the others */
    for (mt=0;mt!=MAXMAGIETYP;++mt) {
      if (mt!=mtype) {
        sn = get_spellnames(lang, mt);
        if (findtoken(&sn->names, name, &token)!=E_TOK_NOMATCH) break;
      }
    }
  }

  if (token.v!=NULL) return (spell*)token.v;
  if (lang==default_locale) return NULL;
  return get_spellfromtoken_i(name, default_locale, mtype);
}

spell *
get_spellfromtoken(unit *u, const char *name, const struct locale * lang)
{
  sc_mage * m = get_mage(u);
  spell * sp;

  if (m==NULL) return NULL;
  sp = get_spellfromtoken_i(name, lang, m->magietyp);
  if (sp!=NULL) {
    spell_list * slist = m->spells;

    while (slist && slist->data->id<=sp->id) {
      if (sp==slist->data) return sp;
      slist = slist->next;
    }
  }
  return NULL;
}

spell *
find_spellbyid(magic_t mtype, spellid_t id)
{
  spell_list * slist;

  assert(id>=0);
#ifndef SHOWASTRAL_NOT_BORKED
  /* disabled spells */
  if (id==SPL_SHOWASTRAL) return NULL;
#endif
  if (id==SPL_NOSPELL) return NULL;
  for (slist=spells;slist!=NULL;slist=slist->next) {
    spell* sp = slist->data;
    if (sp->id == id) return sp;
  }
  for (slist=spells;slist!=NULL;slist=slist->next) {
    spell* sp = slist->data;
    unsigned int hashid = hashstring(sp->sname);
    if (hashid==id) {
      if (sp->magietyp==mtype || mtype==M_GRAU) {
        return sp;
      }
    }
  }  

  log_warning(("cannot find spell by id: %u\n", id));
  return NULL;
}

