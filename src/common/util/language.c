/* vi: set ts=2:
 *
 *	$Id: language.c,v 1.4 2001/02/11 20:54:01 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "goodies.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define SMAXHASH 512

/** importing **/

struct locale {
	struct locale * next;
	unsigned int hashkey;
	const char * name;
	struct locale_string {
		unsigned int hashkey;
		struct locale_string * nexthash;
		char * str;
		char * key;
	} * strings[SMAXHASH];
};

static locale * locales;
static locale * default_locale;

unsigned int
locale_hashkey(const locale * lang)
{
	if (lang==NULL) lang = default_locale;
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

const char *
locale_string(const locale * lang, const char * key)
{
	unsigned int hkey = hashstring(key);
	unsigned int id = hkey % SMAXHASH;
	struct locale_string * find;

	if (key==NULL || *key==0) return NULL;
	if (lang==NULL) lang = default_locale;
	find = lang->strings[id];
	while (find) {
		if (find->hashkey == hkey && !strcmp(key, find->key)) break;
		find = find->nexthash;
	}
	if (!find) {
		if (lang==default_locale) {
			fprintf(stderr, "WARNING: missing translation for \"%s\"\n", key);
			return key;
		}
		fprintf(stderr, "WARNING: missing translation for \"%s\" in locale %s\n", key, lang->name);
		return locale_string(default_locale, key);
	}
	return find->str;
}

void
locale_setstring(locale * lang, const char * key, const char * value)
{
	int nval = atoi(key);
	unsigned int hkey = nval?nval:hashstring(key);
	unsigned int id = hkey % SMAXHASH;
	struct locale_string * find;

	if (lang==NULL) lang = default_locale;
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
	if (lang==NULL) lang = default_locale;
	return lang->name;
}

const char *
reverse_lookup(const locale * lang, const char * str)
{
	int i;
	if (lang==NULL) lang = default_locale;
	assert(lang!=NULL);
	for (i=0;i!=SMAXHASH;++i) {
		struct locale_string * ls;
		for (ls=lang->strings[i];ls;ls=ls->nexthash) {
			if (strcasecmp(ls->key, str)==0) return ls->key;
			if (strcasecmp(ls->str, str)==0) return ls->key;
		}
	}
	return str;
}
