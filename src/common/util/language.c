/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */
#include <config.h>
#include "language.h"
#include "language_struct.h"

#include "log.h"
#include "goodies.h"
#include "umlaut.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/** importing **/

locale * default_locale;
locale * locales;

unsigned int
locale_hashkey(const locale * lang)
{
	assert(lang);
	return lang->hashkey;
}

locale *
find_locale(const char * name)
{
	unsigned int hkey = hashstring(name);
	locale * l = locales;
	while (l && l->hashkey!=hkey) l=l->next;
	return l;
}

static int nextlocaleindex = 0;

locale *
make_locale(const char * name)
{
	unsigned int hkey = hashstring(name);
	locale * l = (locale *)calloc(sizeof(locale), 1);
	locale ** lp = &locales;

  while (*lp && (*lp)->hashkey!=hkey) lp=&(*lp)->next;
	assert(*lp == NULL);

	l->hashkey = hkey;
	l->name = strdup(name);
	l->next = NULL;
  l->index = nextlocaleindex++;
  assert(nextlocaleindex<=MAXLOCALES);
	*lp = l;
	if (default_locale==NULL) default_locale = l;
	return l;
}

/** creates a list of locales
 * This function takes a comma-delimited list of locale-names and creates
 * the locales using the make_locale function (useful for ini-files).
 * For maximum performance, locales should be created in order of popularity.
 */
void
make_locales(const char * str)
{
  const char * tok = str;
  while (*tok) {
    char zText[32];
    while (*tok && *tok !=',') ++tok;
    strncpy(zText, str, tok-str);
    zText[tok-str] = 0;
    make_locale(zText);
    if (*tok) {
      str = ++tok;
    }
  }
}

static FILE * s_debug = NULL;
static char * s_logfile = NULL;

void
debug_language(const char * log)
{
  s_logfile = strdup(log);
}

const char *
locale_getstring(const locale * lang, const char * key)
{
  unsigned int hkey = hashstring(key);
  unsigned int id = hkey & (SMAXHASH-1);
  const struct locale_str * find;

  assert(lang);
  if (key==NULL || *key==0) return NULL;
  find = lang->strings[id];
  while (find) {
    if (find->hashkey == hkey) {
      if (find->nexthash==NULL) {
        /* if this is the only entry with this hash, fine. */
        assert(strcmp(key, find->key)==0);
        return find->str;
      }
      if (strcmp(key, find->key)==0) {
        return find->str;
      }
    }
    find = find->nexthash;
  }
  return NULL;
}

const char *
locale_string(const locale * lang, const char * key)
{
  if (key!=NULL) {
    unsigned int hkey = hashstring(key);
    unsigned int id = hkey & (SMAXHASH-1);
    struct locale_str * find;
    
    if (key == NULL || *key==0) return NULL;
    if (lang == NULL) return key;
    find = lang->strings[id];
    while (find) {
      if (find->hashkey == hkey) {
        if (find->nexthash==NULL) {
          /* if this is the only entry with this hash, fine. */
          assert(strcmp(key, find->key)==0);
          break;
        }
        if (strcmp(key, find->key)==0) break;
      }
      find = find->nexthash;
    }
    if (!find) {
      const char * s = key;
      log_warning(("missing translation for \"%s\" in locale %s\n", key, lang->name));
      if (lang!=default_locale) {
        s = locale_string(default_locale, key);
      }
      if (s_logfile) {
        s_debug = s_debug?s_debug:fopen(s_logfile, "w+");
        if (s_debug) {
          fprintf(s_debug, "%s;%s;%s\n", key, lang->name, s);
          fflush(s_debug);
          locale_setstring((struct locale*)lang, key, s);
        }
      }
      return s;
    }
    return find->str;
  }
  return NULL;
}

void
locale_setstring(locale * lang, const char * key, const char * value)
{
  unsigned int hkey = hashstring(key);
  unsigned int id = hkey & (SMAXHASH-1);
  struct locale_str * find;
  if (lang==NULL) lang=default_locale;
  find = lang->strings[id];
  while (find) {
    if (find->hashkey==hkey && strcmp(key, find->key)==0) break;
    find = find->nexthash;
  }
  if (!find) {
    find = calloc(1, sizeof(struct locale_str));
    find->nexthash = lang->strings[id];
    lang->strings[id] = find;
    find->hashkey = hkey;
    find->key = strdup(key);
    find->str = strdup(value);
  }
  else {
    if (strcmp(find->str, value)!=0) {
      log_error(("Duplicate key %s for '%s' and '%s'\n", key, value, find->str));
    }
    assert(!strcmp(find->str, value) || !"duplicate string for key");
  }
}

const char *
locale_name(const locale * lang)
{
	assert(lang);
	return lang->name;
}

char *
mkname_buf(const char * space, const char * name, char * buffer)
{
  if (space && *space) {
    sprintf(buffer, "%s::%s", space, name);
  } else {
    strcpy(buffer, name);
  }
  return buffer;
}

const char *
mkname(const char * space, const char * name)
{
  static char zBuffer[128];
  return mkname_buf(space, name, zBuffer);
}

locale *
nextlocale(const struct locale * lang)
{
	return lang->next;
}

typedef struct lstr {
  tnode tokens[UT_MAX];
} lstr;

static lstr lstrs[MAXLOCALES];

struct tnode *
get_translations(const struct locale * lang, int index)
{
  assert(lang->index<MAXLOCALES || "you have to increase MAXLOCALES and recompile");
  if (lang->index<MAXLOCALES) {
    return lstrs[lang->index].tokens+index;
  }
  return lstrs[0].tokens+index;
}
