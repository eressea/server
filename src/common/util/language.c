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

locale *
make_locale(const char * name)
{
	unsigned int hkey = hashstring(name);
	locale * l = calloc(sizeof(locale), 1);
#ifndef NDEBUG
	locale * lp = locales;
	while (lp && lp->hashkey!=hkey) lp=lp->next;
	assert(lp == NULL);
#endif
	l->hashkey = hkey;
	l->name = strdup(name);
	l->next = locales;
	locales = l;
	if (default_locale==NULL) default_locale = l;
	return l;
}

static FILE * s_debug = NULL;

void
debug_language(const char * log)
{
	s_debug = fopen(log, "w+");
}

const char *
locale_getstring(const locale * lang, const char * key)
{
	unsigned int hkey = hashstring(key);
	unsigned int id = hkey % SMAXHASH;
	struct locale_string * find;

	assert(lang);
	if (key==NULL || *key==0) return NULL;
	find = lang->strings[id];
	while (find) {
		if (find->hashkey == hkey && !strcmp(key, find->key)) return find->str;
		find = find->nexthash;
	}
	return NULL;
}

const char *
locale_string(const locale * lang, const char * key)
{
	if (key==NULL) return NULL;
	else {
		unsigned int hkey = hashstring(key);
		unsigned int id = hkey % SMAXHASH;
		struct locale_string * find;

		if (key == NULL || *key==0) return NULL;
		if (lang == NULL) return key;
		find = lang->strings[id];
		while (find) {
			if (find->hashkey == hkey && !strcmp(key, find->key)) break;
			find = find->nexthash;
		}
		if (!find) {
			const char * s = key;
			log_warning(("missing translation for \"%s\" in locale %s\n", key, lang->name));
			if (lang!=default_locale) {
				s = locale_string(default_locale, key);
			}
			if (s_debug) {
				fprintf(s_debug, "%s;%s;%s\n", key, lang->name, s);
				fflush(s_debug);
				locale_setstring((struct locale*)lang, key, s);
			}
			return s;
		}
		return find->str;
	}
}

void
locale_setstring(locale * lang, const char * key, const char * value)
{
	int nval = atoi(key);
	unsigned int hkey = nval?nval:hashstring(key);
	unsigned int id = hkey % SMAXHASH;
	struct locale_string * find;

	if (lang==NULL) lang=default_locale;
	find = lang->strings[id];
	while (find) {
		if (find->hashkey==hkey && !strcmp(key, find->key)) break;
		find=find->nexthash;
	}
	if (!find) {
		find = calloc(1, sizeof(struct locale_string));
		find->nexthash = lang->strings[id];
		lang->strings[id] = find;
		find->hashkey = hkey;
		find->key = strdup(key);
		find->str = strdup(value);
	}
	else assert(!strcmp(find->str, value) || !"duplicate string for key");
}

const char *
locale_name(const locale * lang)
{
	assert(lang);
	return lang->name;
}

const char *
reverse_lookup(const locale * lang, const char * str)
{
	int i;
	assert(lang);
	if (strlen(str)) {
		if (lang!=NULL) {
			for (i=0;i!=SMAXHASH;++i) {
				struct locale_string * ls;
				for (ls=lang->strings[i];ls;ls=ls->nexthash) {
					if (strcasecmp(ls->key, str)==0) return ls->key;
					if (strcasecmp(ls->str, str)==0) return ls->key;
				}
			}
		}
		log_warning(("could not do a reverse_lookup for \"%s\" in locale %s\n", str, lang->name));
	}
	return str;
}

const char *
mkname(const char * space, const char * name)
{
	static char zBuffer[128];
	if (space && *space) {
		sprintf(zBuffer, "%s::%s", space, name);
	} else {
		strcpy(zBuffer, name);
	}
	return zBuffer;
}

locale *
nextlocale(const struct locale * lang)
{
	return lang->next;
}
