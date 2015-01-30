/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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
#include "language.h"
#include "language_struct.h"

#include "log.h"
#include "goodies.h"
#include "umlaut.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <critbit.h>

/** importing **/

locale *default_locale;
locale *locales;

unsigned int locale_index(const locale * lang)
{
    assert(lang);
    return lang->index;
}

locale *get_locale(const char *name)
{
    unsigned int hkey = hashstring(name);
    locale *l = locales;
    while (l && l->hashkey != hkey)
        l = l->next;
    return l;
}

static unsigned int nextlocaleindex = 0;

locale *get_or_create_locale(const char *name)
{
    locale *l;
    unsigned int hkey = hashstring(name);
    locale **lp = &locales;

    if (!locales) {
        nextlocaleindex = 0;
    }
    else {
        while (*lp && (*lp)->hashkey != hkey) lp = &(*lp)->next;
        if (*lp) {
            return *lp;
        }
    }
    *lp = l = (locale *)calloc(sizeof(locale), 1);
    l->hashkey = hkey;
    l->name = _strdup(name);
    l->index = nextlocaleindex++;
    assert(nextlocaleindex <= MAXLOCALES);
    if (default_locale == NULL) default_locale = l;
    return l;
}

/** creates a list of locales
 * This function takes a comma-delimited list of locale-names and creates
 * the locales using the make_locale function (useful for ini-files).
 * For maximum performance, locales should be created in order of popularity.
 */
void make_locales(const char *str)
{
    const char *tok = str;
    while (*tok) {
        char zText[32];
        while (*tok && *tok != ',')
            ++tok;
        strncpy(zText, str, tok - str);
        zText[tok - str] = 0;
        get_or_create_locale(zText);
        if (*tok) {
            str = ++tok;
        }
    }
}

const char *locale_getstring(const locale * lang, const char *key)
{
    unsigned int hkey = hashstring(key);
    unsigned int id = hkey & (SMAXHASH - 1);
    const struct locale_str *find;

    assert(lang);
    if (key == NULL || *key == 0)
        return NULL;
    find = lang->strings[id];
    while (find) {
        if (find->hashkey == hkey) {
            if (find->nexthash == NULL) {
                /* if this is the only entry with this hash, fine. */
                assert(strcmp(key, find->key) == 0);
                return find->str;
            }
            if (strcmp(key, find->key) == 0) {
                return find->str;
            }
        }
        find = find->nexthash;
    }
    return NULL;
}

const char *locale_string(const locale * lang, const char *key, bool warn)
{
    assert(lang);
    assert(key);

    if (key != NULL) {
        unsigned int hkey = hashstring(key);
        unsigned int id = hkey & (SMAXHASH - 1);
        struct locale_str *find;

        if (*key == 0) return 0;
        find = lang->strings[id];
        while (find) {
            if (find->hashkey == hkey) {
                if (!find->nexthash) {
                    /* if this is the only entry with this hash, fine. */
                    assert(strcmp(key, find->key) == 0);
                    break;
                }
                if (strcmp(key, find->key) == 0)
                    break;
            }
            find = find->nexthash;
        }
        if (find) {
            return find->str;
        }
        if (warn) {
            log_warning("missing translation for \"%s\" in locale %s\n", key, lang->name);
        }
        if (lang->fallback) {
            return locale_string(lang->fallback, key, warn);
        }
    }
    return 0;
}

void locale_setstring(locale * lang, const char *key, const char *value)
{
    unsigned int hkey = hashstring(key);
    unsigned int id = hkey & (SMAXHASH - 1);
    struct locale_str *find;
    if (!lang) {
        lang = default_locale;
    }
    assert(lang);
    find = lang->strings[id];
    while (find) {
        if (find->hashkey == hkey && strcmp(key, find->key) == 0)
            break;
        find = find->nexthash;
    }
    if (!find) {
        find = calloc(1, sizeof(struct locale_str));
        find->nexthash = lang->strings[id];
        lang->strings[id] = find;
        find->hashkey = hkey;
        find->key = _strdup(key);
        find->str = _strdup(value);
    }
    else {
        if (strcmp(find->str, value) != 0) {
            log_warning("multiple translations for key %s\n", key);
        }
        free(find->str);
        find->str = _strdup(value);
    }
}

const char *locale_name(const locale * lang)
{
    return lang ? lang->name : "(null)";
}

char *mkname_buf(const char *space, const char *name, char *buffer)
{
    if (space && *space) {
        sprintf(buffer, "%s::%s", space, name);
    }
    else {
        strcpy(buffer, name);
    }
    return buffer;
}

const char *mkname(const char *space, const char *name)
{
    static char zBuffer[128]; // FIXME: static return value
    return mkname_buf(space, name, zBuffer);
}

locale *nextlocale(const struct locale * lang)
{
    return lang->next;
}

typedef struct lstr {
    void * tokens[UT_MAX];
} lstr;

static lstr lstrs[MAXLOCALES];

void ** get_translations(const struct locale *lang, int index)
{
    assert(lang);
    assert(lang->index < MAXLOCALES
        || "you have to increase MAXLOCALES and recompile");
    if (lang->index < MAXLOCALES) {
        return lstrs[lang->index].tokens + index;
    }
    return lstrs[0].tokens + index;
}

void add_translation(struct critbit_tree **cbp, const char *key, int i) {
    char buffer[256];
    char * str = transliterate(buffer, sizeof(buffer) - sizeof(int), key);
    struct critbit_tree * cb = *cbp;
    if (str) {
        size_t len = strlen(str);
        if (!cb) {
            // TODO: this will leak, because we do not know how to clean it up */
            *cbp = cb = (struct critbit_tree *)calloc(1, sizeof(struct critbit_tree *));
        }
        len = cb_new_kv(str, len, &i, sizeof(int), buffer);
        cb_insert(cb, buffer, len);
    }
    else {
        log_error("could not transliterate '%s'\n", key);
    }
}

void init_translations(const struct locale *lang, int ut, const char * (*string_cb)(int i), int maxstrings)
{
    void **tokens;
    int i;

    assert(string_cb);
    assert(maxstrings > 0);
    tokens = get_translations(lang, ut);
    for (i = 0; i != maxstrings; ++i) {
        const char * s = string_cb(i);
        const char * key = s ? locale_string(lang, s, false) : 0;
        key = key ? key : s;
        if (key) {
            struct critbit_tree ** cb = (struct critbit_tree **)tokens;
            add_translation(cb, key, i);
        }
    }
}

void *get_translation(const struct locale *lang, const char *str, int index) {
    void **tokens = get_translations(lang, index);
    variant var;
    if (findtoken(*tokens, str, &var) == E_TOK_SUCCESS) {
        return var.v;
    }
    return NULL;
}

const char *localenames[] = {
    "de", "en",
    NULL
};

extern void init_locale(struct locale *lang);

static int locale_init = 0;

void init_locales(void)
{
    int l;
    if (locale_init) return;
    for (l = 0; localenames[l]; ++l) {
        struct locale *lang = get_or_create_locale(localenames[l]);
        init_locale(lang);
    }
    locale_init = 1;
}

void reset_locales(void) {
    locale_init = 0;
}

void free_locales(void) {
    locale_init = 0;
    while (locales) {
        int i;
        locale * next = locales->next;

        for (i = 0; i != SMAXHASH; ++i) {
            while (locales->strings[i]) {
                struct locale_str * strings = locales->strings[i];
                free(strings->key);
                free(strings->str);
                locales->strings[i] = strings->nexthash;
                free(strings);
            }
        }
        free(locales->name);
        free(locales);
        locales = next;
    }
    memset(lstrs, 0, sizeof(lstrs)); // TODO: does this data need to be free'd?
}
